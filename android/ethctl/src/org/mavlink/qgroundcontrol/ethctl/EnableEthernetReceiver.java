package org.mavlink.qgroundcontrol.ethctl;

import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

/**
 * targetSdk 22 helper: Android 8+ SettingsProvider throws when a targetSdk&gt;22 app writes OEM
 * System keys such as {@code isEthernetOpen}. This package targets API 22 so the write is allowed
 * (with WRITE_SETTINGS granted at install).
 */
public final class EnableEthernetReceiver extends BroadcastReceiver {
    private static final String TAG = "QGCEthCtl";
    private static final Uri SYSTEM = Uri.parse("content://settings/system");
    private static final String KEY_OPEN = "isEthernetOpen";
    private static final String KEY_INIT = "eth_init";

    @Override
    public void onReceive(final Context context, final Intent intent) {
        final boolean okOpen = putSystemInt(context, KEY_OPEN, 1);
        putSystemInt(context, KEY_INIT, 1);
        Log.i(TAG, "enable ethernet switch: isEthernetOpen write=" + okOpen);
    }

    private static boolean putSystemInt(final Context context, final String key, final int value) {
        try {
            final ContentValues values = new ContentValues(1);
            values.put("value", String.valueOf(value));
            final Uri item = SYSTEM.buildUpon().appendPath(key).build();
            if (context.getContentResolver().update(item, values, null, null) > 0) {
                return true;
            }
            values.put("name", key);
            return context.getContentResolver().insert(SYSTEM, values) != null;
        } catch (final Exception e) {
            Log.w(TAG, "put " + key + " failed", e);
            return false;
        }
    }
}
