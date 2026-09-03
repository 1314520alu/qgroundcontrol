package org.mavlink.qgroundcontrol;

import android.content.ComponentName;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;

import java.lang.reflect.Method;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.Enumeration;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Best-effort bring-up of radio Ethernet (eth0 / 192.168.144.x) on handheld GCS remotes.
 * <p>
 * OEM Settings expose an Ethernet switch persisted as {@code isEthernetOpen} in
 * {@link Settings.System}. Skydroid H30 / G-series {@code EthernetServiceImpl} observes that key
 * and starts or stops eth0. SIYI UniRC additionally applies static 192.168.144.20/24 when it
 * receives {@code com.action.eth1_up} ({@code EthernetStaicIPReceiver}).
 * <p>
 * On SIYI remotes, launch/resume turns the switch back on if it is off and retries until the
 * 144 subnet appears. On Skydroid, this helper must not write {@code isEthernetOpen}: the OEM
 * EthernetService NetworkAgent stops the on-device MAVLink UDP bridge, so every GCS loses
 * telemetry while payload RTSP on 192.168.144.x keeps working.
 */
public final class QGCSiyiEthernetHelper {
    private static final String TAG = "QGCSiyiEthernet";

    private static final String IFACE = "eth0";
    private static final String ACTION_ETH_UP = "com.action.eth1_up";
    private static final String SETTINGS_PACKAGE = "com.android.settings";
    private static final String SETTINGS_RECEIVER =
            "com.android.settings.ethernet.EthernetStaicIPReceiver";
    private static final String SUBNET_PREFIX = "192.168.144.";

    private static final String KEY_ETHERNET_OPEN = "isEthernetOpen";
    private static final String KEY_ETH_INIT = "eth_init";

    private static final String DEFAULT_IP = "192.168.144.20";
    private static final String DEFAULT_MASK = "255.255.255.0";
    private static final String DEFAULT_GATEWAY = "192.168.144.14";

    private static final long POLL_INTERVAL_MS = 1000L;
    private static final int MAX_ENSURE_ATTEMPTS = 60; // ~1 minute of polling
    private static final long USB_RECOVERY_DELAY_MS = 800L;

    // Observed on UniRC: internal radio presents as USB CDC Ethernet (cdc_ether).
    private static final int SIYI_RADIO_ETH_VENDOR_ID = 4070;   // 0x0FE6
    private static final int SIYI_RADIO_ETH_PRODUCT_ID = 39168; // 0x9900
    private static final int USB_CLASS_COMM = 2;
    private static final int USB_CDC_SUBCLASS_ETHERNET = 6;

    private static final AtomicBoolean s_ensureInProgress = new AtomicBoolean(false);
    private static final AtomicBoolean s_loggedSettingsWriteDenied = new AtomicBoolean(false);
    private static final Handler s_handler = new Handler(Looper.getMainLooper());
    private static int s_attemptsRemaining = 0;
    /** OEM EthernetStaicIPReceiver delays setStaticIp by 5s and resets on each eth1_up — broadcast sparingly. */
    private static boolean s_didBroadcastEthUp = false;
    private static boolean s_didBroadcastAfterIface = false;
    private static final Runnable s_usbRecoveryRunnable = QGCSiyiEthernetHelper::ensureRadioEthernet;

    private QGCSiyiEthernetHelper() {
    }

    /**
     * True when this USB device is the SIYI radio ethernet gadget (must not be claimed as serial/HID).
     */
    public static boolean isRadioEthernetUsbDevice(final android.hardware.usb.UsbDevice device) {
        if (device == null) {
            return false;
        }
        if (device.getVendorId() == SIYI_RADIO_ETH_VENDOR_ID
                && device.getProductId() == SIYI_RADIO_ETH_PRODUCT_ID) {
            return true;
        }
        try {
            for (int i = 0; i < device.getInterfaceCount(); i++) {
                final android.hardware.usb.UsbInterface iface = device.getInterface(i);
                if (iface.getInterfaceClass() == USB_CLASS_COMM
                        && iface.getInterfaceSubclass() == USB_CDC_SUBCLASS_ETHERNET) {
                    return true;
                }
            }
        } catch (final Exception ignored) {
        }
        return false;
    }

    /**
     * USB host topology changed (OTG/PC cable plug, radio eth re-enumerate). Re-apply eth0 config shortly.
     */
    public static void onUsbTopologyChanged() {
        final Context context = activityContext();
        if (context == null) {
            return;
        }
        if (!looksLikeSiyiRemote(context) && !looksLikeSkydroidRemote(context)) {
            return;
        }
        QGCLogger.i(TAG, "USB topology changed; scheduling radio ethernet recovery");
        s_handler.removeCallbacks(s_usbRecoveryRunnable);
        s_handler.postDelayed(s_usbRecoveryRunnable, USB_RECOVERY_DELAY_MS);
    }

    /**
     * True when any local IPv4 address is on the SIYI radio subnet (192.168.144.x).
     */
    public static boolean isRadioEthernetReady() {
        return !radioEthernetAddress().isEmpty();
    }

    /**
     * Local IPv4 on 192.168.144.x, or empty if the radio subnet is not up.
     */
    public static String radioEthernetAddress() {
        try {
            final Enumeration<NetworkInterface> ifaces = NetworkInterface.getNetworkInterfaces();
            if (ifaces == null) {
                return "";
            }
            for (final NetworkInterface nif : Collections.list(ifaces)) {
                for (final InetAddress addr : Collections.list(nif.getInetAddresses())) {
                    if (!(addr instanceof Inet4Address) || addr.isLoopbackAddress()) {
                        continue;
                    }
                    final String host = addr.getHostAddress();
                    if (host != null && host.startsWith(SUBNET_PREFIX)) {
                        return host;
                    }
                }
            }
        } catch (final Exception e) {
            QGCLogger.w(TAG, "radioEthernetAddress enumeration failed", e);
        }

        try {
            final Context context = activityContext();
            if (context == null) {
                return "";
            }
            final ConnectivityManager cm =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) {
                return "";
            }
            for (final Network network : cm.getAllNetworks()) {
                final NetworkCapabilities caps = cm.getNetworkCapabilities(network);
                if (caps == null || !caps.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
                    continue;
                }
                final LinkProperties props = cm.getLinkProperties(network);
                if (props == null) {
                    continue;
                }
                for (final LinkAddress linkAddress : props.getLinkAddresses()) {
                    final InetAddress addr = linkAddress.getAddress();
                    if (!(addr instanceof Inet4Address)) {
                        continue;
                    }
                    final String host = addr.getHostAddress();
                    if (host != null && host.startsWith(SUBNET_PREFIX)) {
                        return host;
                    }
                }
            }
        } catch (final Exception e) {
            QGCLogger.w(TAG, "radioEthernetAddress ConnectivityManager failed", e);
        }

        return "";
    }

    /**
     * Skydroid H30-class: USB {@code eth0} on 192.168.144.x carries MAVLink from the air unit
     * (QGC listens on 14550, peer 192.168.144.101:14550). G20/G16-class AR8030 radios use
     * {@code ar_net0} and a localhost UDP bridge (listen 14551, peer 127.0.0.1:14552).
     */
    public static boolean skydroidUsesDirectRadioEthernetTelemetry() {
        try {
            final NetworkInterface eth = NetworkInterface.getByName(IFACE);
            if (eth == null || !eth.isUp()) {
                return false;
            }
            for (final InetAddress addr : Collections.list(eth.getInetAddresses())) {
                if (!(addr instanceof Inet4Address) || addr.isLoopbackAddress()) {
                    continue;
                }
                final String host = addr.getHostAddress();
                if (host != null && host.startsWith(SUBNET_PREFIX)) {
                    return true;
                }
            }
        } catch (final Exception e) {
            QGCLogger.w(TAG, "skydroidUsesDirectRadioEthernetTelemetry failed", e);
        }
        return false;
    }

    /**
     * Kick OEM ethernet bring-up and keep retrying until the 144 subnet is ready or attempts expire.
     * Safe to call repeatedly; overlapping calls extend the attempt budget.
     */
    public static void ensureRadioEthernet() {
        final Context context = activityContext();
        if (context == null) {
            QGCLogger.w(TAG, "ensureRadioEthernet: no activity context");
            return;
        }

        // Skydroid H30: writing Settings.System isEthernetOpen starts EthernetService
        // NetworkAgent and the OEM drops the on-device MAVLink UDP bridge — every GCS
        // then shows disconnected while RTSP on 192.168.144.x still works. Do not touch
        // that switch here. SIYI still needs the toggle / 144.x bring-up below.
        unbindProcessFromNetwork(context);

        final boolean skydroid = looksLikeSkydroidRemote(context);
        if (skydroid && !hasSiyiRadioPackages(context)) {
            QGCLogger.d(TAG, "Skydroid remote: skip ethernet switch (OEM switch kills radio telemetry)");
            return;
        }

        if (!looksLikeSiyiRemote(context)) {
            QGCLogger.d(TAG, "Skipping radio ethernet ensure (not a SIYI remote)");
            return;
        }

        if (isRadioEthernetReady()) {
            QGCLogger.i(TAG, "Radio ethernet already ready: " + radioEthernetAddress());
            s_ensureInProgress.set(false);
            s_handler.removeCallbacks(s_usbRecoveryRunnable);
            return;
        }

        s_attemptsRemaining = MAX_ENSURE_ATTEMPTS;
        s_didBroadcastEthUp = false;
        s_didBroadcastAfterIface = false;
        if (!s_ensureInProgress.compareAndSet(false, true)) {
            QGCLogger.d(TAG, "ensureRadioEthernet: already in progress; attempt budget refreshed");
            return;
        }

        QGCLogger.i(TAG, "Starting radio ethernet ensure loop");
        runEnsureAttempt(context.getApplicationContext());
    }

    /** True when kernel has eth0 (may still lack 144.x address). */
    private static boolean hasEth0Interface() {
        return new java.io.File("/sys/class/net/" + IFACE).exists();
    }

    private static void runEnsureAttempt(final Context appContext) {
        if (isRadioEthernetReady()) {
            QGCLogger.i(TAG, "Radio ethernet ready: " + radioEthernetAddress());
            s_ensureInProgress.set(false);
            return;
        }

        if (s_attemptsRemaining <= 0) {
            QGCLogger.w(TAG, "Radio ethernet ensure timed out (eth0 USB gadget may be down / air unit offline)");
            s_ensureInProgress.set(false);
            return;
        }

        s_attemptsRemaining--;
        try {
            maybeSeedSettings(appContext);
            applyEthernetConfiguration(appContext);
            tryEnableEthernet(appContext);

            // Broadcast at most twice: once at start, once when eth0 appears without an address.
            // Repeating every few seconds resets Settings' 5s delayed setStaticIp and delays IP bring-up.
            final boolean hasIface = hasEth0Interface();
            if (!s_didBroadcastEthUp) {
                broadcastEthUp(appContext);
                s_didBroadcastEthUp = true;
            } else if (hasIface && !s_didBroadcastAfterIface) {
                QGCLogger.i(TAG, "eth0 present without 144 address; re-broadcast eth1_up once");
                broadcastEthUp(appContext);
                s_didBroadcastAfterIface = true;
            }
        } catch (final Exception e) {
            QGCLogger.w(TAG, "ensure attempt failed", e);
        }

        s_handler.postDelayed(() -> runEnsureAttempt(appContext), POLL_INTERVAL_MS);
    }

    /**
     * Restore the OEM Ethernet toggle if it is off.
     * <p>
     * Skydroid EthernetSettings writes {@code Settings.System isEthernetOpen}; {@code EthernetServiceImpl}
     * observes that key and starts/stops eth0. SIYI remotes keep the same key in System and/or Secure.
     */
    private static void ensureEthernetSwitchOn(final Context context) {
        final int value = ethernetSwitchValue(context);
        if (value == 1) {
            QGCLogger.d(TAG, "Ethernet switch already on");
            return;
        }

        final boolean knownRemote = looksLikeSiyiRemote(context) || looksLikeSkydroidRemote(context);
        if (value != 0 && !knownRemote) {
            return;
        }

        QGCLogger.i(TAG, "Ethernet switch is " + (value == 0 ? "off" : "unknown") + "; turning on");
        final boolean wroteSystem = putSystemInt(context, KEY_ETHERNET_OPEN, 1);
        putSystemInt(context, KEY_ETH_INIT, 1);
        putSecureInt(context, KEY_ETHERNET_OPEN, 1);
        tryEnableEthernet(context);
        if (!wroteSystem) {
            requestHelperEnableEthernet(context);
        }
        if (!wroteSystem && value == 0) {
            QGCLogger.w(TAG, "Could not write Settings.System isEthernetOpen=1; asked ethctl helper");
        }
    }

    /**
     * Clear the process default Network so dual-home sockets (eth0 192.168.144.x + lo) all work.
     * Call before Qt creates UDP/RTSP sockets ({@code Activity.onCreate} before {@code super}).
     */
    public static void unbindProcessFromNetwork(final Context context) {
        if (context == null) {
            return;
        }
        try {
            final ConnectivityManager cm =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) {
                return;
            }
            cm.bindProcessToNetwork(null);
            QGCLogger.d(TAG, "bindProcessToNetwork(null) so sockets are not pinned to one Network");
        } catch (final Exception e) {
            QGCLogger.d(TAG, "unbindProcessFromNetwork: " + e.getMessage());
        }
    }

    /**
     * targetSdk 22 helper APK can write OEM System keys that this app (targetSdk 36) cannot.
     */
    private static void requestHelperEnableEthernet(final Context context) {
        try {
            final Intent intent = new Intent("org.mavlink.qgroundcontrol.ethctl.ENABLE_ETHERNET");
            intent.setComponent(new ComponentName(
                    "org.mavlink.qgroundcontrol.ethctl",
                    "org.mavlink.qgroundcontrol.ethctl.EnableEthernetReceiver"));
            intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
            context.sendBroadcast(intent);
            QGCLogger.i(TAG, "Broadcast ENABLE_ETHERNET to ethctl helper");
        } catch (final Exception e) {
            QGCLogger.w(TAG, "ethctl helper broadcast failed: " + e.getMessage());
        }
    }

    /** 1 = on, 0 = off, -1 = setting absent. */
    private static int ethernetSwitchValue(final Context context) {
        try {
            final int system = Settings.System.getInt(context.getContentResolver(), KEY_ETHERNET_OPEN, -1);
            if (system == 0 || system == 1) {
                return system;
            }
            return Settings.Secure.getInt(context.getContentResolver(), KEY_ETHERNET_OPEN, -1);
        } catch (final Exception e) {
            QGCLogger.d(TAG, "ethernetSwitchValue failed: " + e.getClass().getSimpleName());
            return -1;
        }
    }

    private static boolean looksLikeSiyiRemote(final Context context) {
        final PackageManager pm = context.getPackageManager();
        return hasSiyiRadioPackages(context)
                || isPackageInstalled(pm, "com.example.zyhkgcsandroid");
    }

    private static boolean hasSiyiRadioPackages(final Context context) {
        final PackageManager pm = context.getPackageManager();
        return isPackageInstalled(pm, "com.siyi.udpservice")
                || isPackageInstalled(pm, "biz.siyi.remotecontrol");
    }

    private static boolean looksLikeSkydroidRemote(final Context context) {
        final PackageManager pm = context.getPackageManager();
        if (isPackageInstalled(pm, "com.skydroid.h30tool")
                || isPackageInstalled(pm, "com.skydroid.fly")
                || isPackageInstalled(pm, "com.skydroid.camerafpv")
                || isPackageInstalled(pm, "com.skydroid.rcsdk")
                || isPackageInstalled(pm, "com.skydroid.server")
                || isPackageInstalled(pm, "com.skydroid.skydroidfly")
                || isPackageInstalled(pm, "com.skydroid.fpv")
                || isPackageInstalled(pm, "com.skydroid.rcservice")
                || isPackageInstalled(pm, "com.skydroid.devicetool")
                || isPackageInstalled(pm, "com.skydroid.rc_daemon")) {
            return true;
        }
        final String model = Build.MODEL;
        if (model == null || model.isEmpty()) {
            return false;
        }
        final String lower = model.toLowerCase(java.util.Locale.ROOT);
        return lower.contains("h30") || lower.contains("g20") || lower.contains("g16")
                || lower.contains("h16") || lower.contains("skydroid");
    }

    private static boolean isPackageInstalled(final PackageManager pm, final String packageName) {
        try {
            pm.getPackageInfo(packageName, 0);
            return true;
        } catch (final Exception e) {
            return false;
        }
    }

    private static Context activityContext() {
        return QGCActivity.getInstance();
    }

    private static void maybeSeedSettings(final Context context) {
        // Flip the OEM switch in System (what EthernetServiceImpl observes). Do not overwrite an
        // existing static IP — Skydroid H30 uses 192.168.144.100, SIYI UniRC uses .20.
        putSystemInt(context, KEY_ETHERNET_OPEN, 1);
        putSystemInt(context, KEY_ETH_INIT, 1);

        if (s_loggedSettingsWriteDenied.get()) {
            return;
        }
        final boolean wrote = putSecureInt(context, KEY_ETHERNET_OPEN, 1)
                && putSecureInt(context, "isEthernetStaticOpen", 1)
                && putSecureString(context, "ethernet_static_ip", DEFAULT_IP)
                && putSecureString(context, "ethernet_static_netmask", DEFAULT_MASK)
                && putSecureString(context, "ethernet_static_gateway", DEFAULT_GATEWAY);
        if (!wrote) {
            s_loggedSettingsWriteDenied.set(true);
            QGCLogger.d(TAG, "Cannot write ethernet settings (expected); relying on OEM System switch + eth1_up");
        }
    }

    private static void broadcastEthUp(final Context context) {
        try {
            // Explicit component avoids Android 8+ background broadcast restrictions on implicit intents.
            final Intent intent = new Intent(ACTION_ETH_UP);
            intent.setComponent(new ComponentName(SETTINGS_PACKAGE, SETTINGS_RECEIVER));
            context.sendBroadcast(intent);
            QGCLogger.d(TAG, "Broadcast " + ACTION_ETH_UP + " -> " + SETTINGS_RECEIVER);
        } catch (final Exception e) {
            QGCLogger.w(TAG, "Failed to broadcast " + ACTION_ETH_UP, e);
        }
    }

    /**
     * Mirror SIYI EthernetUtil / Settings EthernetStaicIPReceiver via hidden EthernetManager APIs.
     */
    private static void applyEthernetConfiguration(final Context context) {
        try {
            final Object ethManager = context.getSystemService("ethernet");
            if (ethManager == null) {
                QGCLogger.d(TAG, "EthernetManager unavailable");
                return;
            }

            final Class<?> ipConfigClass = Class.forName("android.net.IpConfiguration");
            final Object ipConfig = ipConfigClass.getDeclaredConstructor().newInstance();

            final Class<?> ipAssignmentClass = Class.forName("android.net.IpConfiguration$IpAssignment");
            @SuppressWarnings({"unchecked", "rawtypes"})
            final Object staticAssignment = Enum.valueOf((Class) ipAssignmentClass, "STATIC");
            ipConfigClass.getMethod("setIpAssignment", ipAssignmentClass).invoke(ipConfig, staticAssignment);

            final Class<?> proxyClass = Class.forName("android.net.IpConfiguration$ProxySettings");
            @SuppressWarnings({"unchecked", "rawtypes"})
            final Object proxyStatic = Enum.valueOf((Class) proxyClass, "STATIC");
            ipConfigClass.getMethod("setProxySettings", proxyClass).invoke(ipConfig, proxyStatic);

            final Object staticIp = buildStaticIpConfiguration(context);
            if (staticIp != null) {
                final Class<?> staticIpClass = Class.forName("android.net.StaticIpConfiguration");
                ipConfigClass.getMethod("setStaticIpConfiguration", staticIpClass).invoke(ipConfig, staticIp);
            }

            Method setConfiguration = null;
            for (final Method method : ethManager.getClass().getMethods()) {
                if ("setConfiguration".equals(method.getName()) && method.getParameterTypes().length == 2) {
                    setConfiguration = method;
                    break;
                }
            }
            if (setConfiguration == null) {
                QGCLogger.d(TAG, "EthernetManager.setConfiguration not found");
                return;
            }
            setConfiguration.setAccessible(true);
            setConfiguration.invoke(ethManager, IFACE, ipConfig);
            QGCLogger.i(TAG, "EthernetManager.setConfiguration(" + IFACE + ") invoked");
        } catch (final Exception e) {
            // Expected on non-privileged apps; OEM broadcast path remains the primary mechanism.
            QGCLogger.d(TAG, "EthernetManager.setConfiguration unavailable: " + e.getClass().getSimpleName());
        }
    }

    private static Object buildStaticIpConfiguration(final Context context) throws Exception {
        final String ip = getSecureOrDefault(context, "ethernet_static_ip", DEFAULT_IP);
        final String mask = getSecureOrDefault(context, "ethernet_static_netmask", DEFAULT_MASK);
        final String gateway = getSecureOrDefault(context, "ethernet_static_gateway", DEFAULT_GATEWAY);

        final InetAddress ipAddr = InetAddress.getByName(ip);
        final InetAddress gwAddr = InetAddress.getByName(gateway);
        final int prefix = netmaskToPrefix(mask);
        final Object linkAddress = newLinkAddress(ipAddr, prefix);

        try {
            final Class<?> builderClass = Class.forName("android.net.StaticIpConfiguration$Builder");
            final Object builder = builderClass.getDeclaredConstructor().newInstance();
            builderClass.getMethod("setIpAddress", Class.forName("android.net.LinkAddress")).invoke(builder, linkAddress);
            builderClass.getMethod("setGateway", InetAddress.class).invoke(builder, gwAddr);
            return builderClass.getMethod("build").invoke(builder);
        } catch (final ClassNotFoundException e) {
            final Class<?> staticIpClass = Class.forName("android.net.StaticIpConfiguration");
            final Object staticIp = staticIpClass.getDeclaredConstructor().newInstance();
            staticIpClass.getField("ipAddress").set(staticIp, linkAddress);
            staticIpClass.getField("gateway").set(staticIp, gwAddr);
            return staticIp;
        }
    }

    /** LinkAddress(InetAddress, int) is not always visible in the public SDK stubs — use reflection. */
    private static Object newLinkAddress(final InetAddress address, final int prefixLength) throws Exception {
        final Class<?> linkAddressClass = Class.forName("android.net.LinkAddress");
        return linkAddressClass
                .getConstructor(InetAddress.class, int.class)
                .newInstance(address, prefixLength);
    }

    private static void tryEnableEthernet(final Context context) {
        try {
            final Object ethManager = context.getSystemService("ethernet");
            if (ethManager == null) {
                return;
            }
            try {
                final Method enabled = ethManager.getClass().getMethod("setEthernetEnabled", boolean.class);
                enabled.invoke(ethManager, true);
                QGCLogger.d(TAG, "setEthernetEnabled(true)");
                return;
            } catch (final NoSuchMethodException ignored) {
            }
            try {
                final Method enabledIface =
                        ethManager.getClass().getMethod("setEthernetEnabled", String.class, boolean.class);
                enabledIface.invoke(ethManager, IFACE, true);
                QGCLogger.d(TAG, "setEthernetEnabled(" + IFACE + ", true)");
                return;
            } catch (final NoSuchMethodException ignored) {
            }
            try {
                final Method enable = ethManager.getClass().getMethod("enable", String.class);
                enable.invoke(ethManager, IFACE);
                QGCLogger.d(TAG, "enable(" + IFACE + ")");
            } catch (final NoSuchMethodException ignored) {
            }
        } catch (final Exception e) {
            QGCLogger.d(TAG, "tryEnableEthernet: " + e.getMessage());
        }
    }

    private static String getSecureOrDefault(final Context context, final String key, final String fallback) {
        try {
            final String value = Settings.Secure.getString(context.getContentResolver(), key);
            if (value != null && !value.isEmpty()) {
                return value;
            }
        } catch (final Exception ignored) {
        }
        try {
            final String value = Settings.System.getString(context.getContentResolver(), key);
            if (value != null && !value.isEmpty()) {
                return value;
            }
        } catch (final Exception ignored) {
        }
        return fallback;
    }

    private static boolean putSecureInt(final Context context, final String key, final int value) {
        try {
            return Settings.Secure.putInt(context.getContentResolver(), key, value);
        } catch (final Exception e) {
            return false;
        }
    }

    private static boolean putSystemInt(final Context context, final String key, final int value) {
        // Settings.System.putInt throws IllegalArgumentException on API 26+ for OEM keys that are
        // not in PUBLIC_SETTINGS (isEthernetOpen, eth_init). Write the SettingsProvider URI directly
        // so EthernetOpenedObserver still fires.
        try {
            final ContentValues values = new ContentValues(1);
            values.put("value", String.valueOf(value));
            final Uri uri = Settings.System.getUriFor(key);
            final int updated = context.getContentResolver().update(uri, values, null, null);
            if (updated > 0) {
                return true;
            }
            values.put("name", key);
            final Uri inserted = context.getContentResolver().insert(Settings.System.CONTENT_URI, values);
            return inserted != null;
        } catch (final Exception e) {
            QGCLogger.d(TAG, "putSystemInt " + key + " failed: " + e.getClass().getSimpleName()
                    + " " + e.getMessage());
            return false;
        }
    }

    private static boolean putSecureString(final Context context, final String key, final String value) {
        try {
            return Settings.Secure.putString(context.getContentResolver(), key, value);
        } catch (final Exception e) {
            return false;
        }
    }

    private static int netmaskToPrefix(final String netmask) {
        try {
            final byte[] bytes = InetAddress.getByName(netmask).getAddress();
            int prefix = 0;
            for (final byte b : bytes) {
                prefix += Integer.bitCount(b & 0xff);
            }
            return prefix;
        } catch (final Exception e) {
            return 24;
        }
    }
}
