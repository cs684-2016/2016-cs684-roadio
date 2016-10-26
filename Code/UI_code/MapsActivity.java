package com.example.eskay.roadio_iitb;

/* 
 * Author List: Mustafa Lokhandwala, Shalaka Kulkarni, Umang Chhaparia
 * Filename: MapsActivity.java
 * Functions: onCreate(Bundle), onMapReady(GoogleMap), setMarkerColor(Marker, float), statusCheck(), buildAlertMessageNoGps(), readFile() 
 * Global Variables: mMap, location[], score[], numberOfLocations.
 */


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
    LatLng location[];
    float score[];
    int numberOfLocations;

    /*
     * Function name: onCreate(Bundle savedInstanceState)
     * Input: savedInstanceState -> last saved bundle
     * Output: None
     * Logic: Initialises activity
     * Example call: Called by default when the activity is launched.
     */
    
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

    /*
     * Function name: onMapReady(GoogleMap googleMap)
     * Input: googleMap -> Google map instance
     * Output: None
     * Logic: Enables location access with user permission; calls function to fetch location and score; adds markers to map.
     * Example call: Called by default when the map has been loaded.
     */
    
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
        
        readFile(); //location and score arrays populated with file contents

        for(int i=0;i<numberOfLocations;i++)
        {
            Marker marker = mMap.addMarker(new MarkerOptions().position(location[i]).title("Road quality score: " + score[i]));
            mMap.moveCamera(CameraUpdateFactory.newLatLng(location[i]));
            setMarkerColor(marker, score[i]);
            Log.d("onMapReady", "Created marker "+i);
        }
    }

     /*
     * Function name: setMarkerColor(Marker marker, float num)
     * Input: marker -> Google map marker instance; num -> score associated with that marker
     * Output: None
     * Logic: Sets marker colour based on the value of num.
     * Example call: Called by default when the map has been loaded.
     */
    
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

     /*
     * Function name: statusCheck()
     * Input: none
     * Output: none
     * Logic: Checks if location has been enabled, if not, enables with user permission.
     * Example call: statusCheck();
     */
    
    public void statusCheck() {
        final LocationManager manager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);

        if (!manager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
            buildAlertMessageNoGps();

        }
    }

     /*
     * Function name: buildAlertMessageNoGps()
     * Input: none
     * Output: none
     * Logic: Creates user alert to launch location activity
     * Example call: buildAlertMessageNoGps();
     */
    
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
    
     /*
     * Function name: readFile()
     * Input: none
     * Output: none
     * Logic: Reads locations and road quality scores from a file myFile.txt received from the post-processing pipeline.
     * Example call: readFile();
     */

    void readFile()
    {
        File filePath = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS));
        filePath.mkdirs();
        File file = new File(filePath, "myFile.txt");

        //File file = new File("file:///sdcard/Download/myFile.txt");

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
     * [done] Read file of lat/lon/score
     * [done] Create array/list of locations and scores
     * [done] Create corresponding array of markers with scores
     * [done] Set colour of marker based on score
     * [done] Display
     * [done] Show own location
     * [done] Ask for permission to enable location if not enabled
     */
}
