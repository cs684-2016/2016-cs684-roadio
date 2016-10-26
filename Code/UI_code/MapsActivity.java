package com.example.eskay.roadio_iitb;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.LocationManager;
import android.os.Environment;
import android.support.v4.app.FragmentActivity;
import android.os.Bundle;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.BitmapDescriptorFactory;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;

import java.io.*;
import java.util.*;

public class MapsActivity extends AppCompatActivity implements OnMapReadyCallback {

    private GoogleMap mMap;
    LatLng location[] = {new LatLng(19.132918, 72.917845), new LatLng(19.132898,72.91785), new LatLng(19.132837,72.917861), new LatLng(19.132766,72.917877), new LatLng(19.13271,72.917883), new LatLng(19.13267,72.917893), new LatLng(19.132634,72.917899), new LatLng(19.132569,72.917963), new LatLng(19.132447,72.917931), new LatLng(19.132406,72.917936)};
    float score[] = {3, 5, 4, 6, 7, 5, 7, 8, 6, 9};
    int numberOfLocations = 10;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_maps);
        setTitle("Roadio-IITB Road Quality Map");
        // Obtain the SupportMapFragment and get notified when the map is ready to be used.
        SupportMapFragment mapFragment = (SupportMapFragment) getSupportFragmentManager()
                .findFragmentById(R.id.map);
        mapFragment.getMapAsync(this);
    }

    @Override
    public void onMapReady(GoogleMap googleMap) {
        mMap = googleMap;
        statusCheck();
        try {
            mMap.setMyLocationEnabled(true);
        }
        catch (SecurityException se) {
            Log.d("onMapReady", "SecurityException");
        }
        /*LatLng loc1 = new LatLng(19.0760, 72.8777);
        mMap.addMarker(new MarkerOptions().position(loc1).title("Mumbai").snippet("Road quality score: 10.0"));
        mMap.moveCamera(CameraUpdateFactory.newLatLng(loc1));*/

        //readFile(); //location and score arrays populated with file contents

        for(int i=0;i<numberOfLocations;i++)
        {
            Marker marker = mMap.addMarker(new MarkerOptions().position(location[i]).title("Road quality score: " + score[i]));
            mMap.moveCamera(CameraUpdateFactory.newLatLng(location[i]));
            setMarkerColor(marker, score[i]);
            Log.d("onMapReady", "Created marker "+i);
        }
    }

    void setMarkerColor(Marker marker, float num)
    {
        int n = (int)num;
        switch(n)
        {
            case 9:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_VIOLET));
                break;
            case 8:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_AZURE));
                break;
            case 7:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_CYAN));
                break;
            case 6:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_BLUE));
                break;
            case 5:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_GREEN));
                break;
            case 4:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_YELLOW));
                break;
            case 3:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_ORANGE));
                break;
            case 2:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_ROSE));
                break;
            case 1:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_MAGENTA));
                break;
            default:
                marker.setIcon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_RED));
        }
    }

    public void statusCheck() {
        final LocationManager manager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);

        if (!manager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
            buildAlertMessageNoGps();

        }
    }

    private void buildAlertMessageNoGps() {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage("Your GPS seems to be disabled, do you want to enable it?")
                .setCancelable(false)
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(final DialogInterface dialog, final int id) {
                        startActivity(new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS));
                    }
                })
                .setNegativeButton("No", new DialogInterface.OnClickListener() {
                    public void onClick(final DialogInterface dialog, final int id) {
                        dialog.cancel();
                    }
                });
        final AlertDialog alert = builder.create();
        alert.show();
    }

    void readFile()
    {
        /*File filePath = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "Embedded");
        filePath.mkdirs();
        File file = new File(filePath, "myFile.txt");*/

        File file = new File("file:///sdcard/Download/myFile.txt");

        try
        {
            BufferedReader br = new BufferedReader(new FileReader(file));
            Log.d("readFile", "File opened");

            String line;
            line = br.readLine();

            numberOfLocations = Integer.parseInt(line);
            location = new LatLng[numberOfLocations];
            score = new float[numberOfLocations];
            Log.d("readFile", "numberOfLocations = "+numberOfLocations);

            for(int i=0; i<numberOfLocations; i++)
            {
                line = br.readLine();
                if(line == null) break;

                //stored as lat + " " + lon + " " + score
                int a1 = line.indexOf(' ');
                int a2 = line.lastIndexOf(' ');
                float lat = Float.parseFloat(line.substring(0,a1));
                float lon = Float.parseFloat(line.substring(a1+1,a2));

                location[i] = new LatLng(lat,lon);
                score[i] = Float.parseFloat(line.substring(a2+1));
                Log.d("readFile", "Fetched location "+i);
            }
            br.close();
        }
        catch (FileNotFoundException e)
        {
            Log.d("readFile", "FileNotFoundException");
        }
        catch (IOException ie)
        {
            Log.d("readFile", "IOException");
        }
    }

    /**
     * TO DO
     * Read file of lat/lon/score <---------- FileNotFoundException
     * [done] Create array/list of locations and scores
     * [done] Create corresponding array of markers with scores
     * [done] Set colour of marker based on score
     * [done] Display
     * [done] Show own location
     * [done] Ask for permission to enable location if not enabled
     */
}
