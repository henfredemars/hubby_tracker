package com.example.james.hubbytracker;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;

import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.StringRequest;
import com.android.volley.toolbox.Volley;
import com.google.android.gms.location.Geofence;
import com.google.android.gms.location.GeofencingEvent;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * An {@link Service} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p>
 * Processes GeoIntents and sends updates to the RPi via the web when the reported distance changes.
 *
 */
public class HubbyGeoService extends Service {

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        int distance = -1;
        if (intent != null) {
            GeofencingEvent geofencingEvent = GeofencingEvent.fromIntent(intent);

            // Find innermost and outermost triggering fences
            List<Geofence> fences = geofencingEvent.getTriggeringGeofences();
            if (fences != null) {
                int minId = 1000;
                int maxId = -1;
                for (int i = 0; i < fences.size(); i++) {
                    Geofence fence = fences.get(i);
                    int cId = Integer.parseInt(fence.getRequestId());
                    if (cId < minId)
                        minId = cId;
                    if (cId > maxId)
                        maxId = cId;
                }

                if (geofencingEvent.getGeofenceTransition() == Geofence.GEOFENCE_TRANSITION_ENTER) {
                    // Take the minimum as the current distance, we just entered it
                    distance = minId;
                } else if (geofencingEvent.getGeofenceTransition() == Geofence.GEOFENCE_TRANSITION_EXIT) {
                    // We just left the greatest range, so we are in the one that is one larger
                    distance = maxId + 1;
                }

                final int innerClassDistance = distance;

                // Tell the RPi about the change
                SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
                RequestQueue queue = Volley.newRequestQueue(this);
                StringRequest sr = new StringRequest(Request.Method.POST, "http://" + prefs.getString("server_name", "henfred.hopto.org") + ":" +
                        prefs.getString("port", "5155") + "/", new Response.Listener<String>() {
                    @Override
                    public void onResponse(String response) {
                        //...
                    }
                }, new Response.ErrorListener() {
                    @Override
                    public void onErrorResponse(VolleyError error) {
                        //...
                    }
                }) {
                    @Override
                    protected Map<String, String> getParams() {
                        Map<String, String> params = new HashMap<String, String>();
                        params.put("ring", String.valueOf(innerClassDistance));

                        return params;
                    }

                };

                queue.add(sr);

            }

        }

        return START_NOT_STICKY;
    }

    private void createNotificationChannel() {
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = getString(R.string.app_name);
            String description = getString(R.string.app_name);
            int importance = NotificationManager.IMPORTANCE_LOW;
            NotificationChannel channel = new NotificationChannel("0", name, importance);
            channel.setDescription(description);
            // Register the channel with the system; you can't change the importance
            // or other notification behaviors after this
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    private void showNotification() {
        Intent notificationIntent = new Intent(this, SettingsActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0,
                notificationIntent, 0);


        Notification notification = new NotificationCompat.Builder(this, "0")
                .setContentTitle("Hubby Tracker running")
                .setContentText("Running...")
                .setContentIntent(pendingIntent)
                .setOngoing(true)
                .setSmallIcon(R.drawable.)
                .setCategory(Notification.CATEGORY_SERVICE)
                .build();

        startForeground(1,
                notification);

    }

    public void onCreate()
    {
        super.onCreate();
        createNotificationChannel();
        showNotification();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


}
