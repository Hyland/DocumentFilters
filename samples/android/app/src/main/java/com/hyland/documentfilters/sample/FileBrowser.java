// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

package com.hyland.documentfilters.sample;

import android.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.content.Intent;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

import com.perceptive.documentfilters.*;

public class FileBrowser extends AppCompatActivity {
    public static final String FILENAME = "com.hyland.documentfilters.sample.FILENAME";
    public static final String KEY =  "com.hyland.documentfilters.sample.KEY";

    private static DocumentFilters _documentFilters;
    private static HashMap<String, Extractor> _documents = new HashMap<>();

    private ListView mListView;
    private ArrayAdapter<String> mListAdapter;
    private Extractor mExtractor;
    private String mExtractorKey;

    private ArrayList<FileInfo> mFiles = new ArrayList<>();

    private class FileInfo {
        public String name;
        public String id;

        public FileInfo(String name, String id) {
            this.name = name;
            this.id = id;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_browser);

        this.mListView = (ListView) findViewById( R.id.fileList );
        mListView.setTextFilterEnabled(true);

        Intent intent = getIntent();
        if (intent == null || intent.getStringExtra(KEY) == null) {
            loadAssetFileList();
        } else {
            setTitle(intent.getStringExtra(FILENAME));
            String key = intent.getStringExtra(KEY);
            Extractor document = _documents.get(key);
            loadDocumentFileList(document, key);
        }
    }
    @Override
    public void onStop() {
        FileBrowser.closeExtractor(mExtractorKey, mExtractor);
        super.onStop();
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_info:
                new AlertDialog.Builder(this)
                        .setMessage("This application demonstrates using Document Filters to preview documents, and process and extract files from containers.")
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }
    public static DocumentFilters documentFilters() throws IGRException {
        if (_documentFilters == null) {
            _documentFilters = new DocumentFilters();
            _documentFilters.Initialize(DocumentFiltersLicense.ISYS_LICENSE_KEY,".");
        }
        return _documentFilters;
    }
    public static Extractor getExtractor(String key) {
        return _documents.get(key);
    }
    public static void closeExtractor(String key, Extractor extractor) {
        try {
            if (_documents.containsKey(key)) {
                _documents.remove(key);
            }
            if (extractor != null) {
                extractor.Close();
            }
        } catch(Exception e) {

        }
    }
    private void showError(String error) {

    }
    private void view(Extractor document, String name, String key) {
        try {
            if (_documents.containsKey(key))
                _documents.remove(key);
            _documents.put(key, document);

            if (document.getSupportsSubFiles() && !document.getSupportsText() && !document.getSupportsHTML()) {
                // Container
                Intent intent = new Intent(this, FileBrowser.class);
                intent.putExtra(FILENAME, name);
                intent.putExtra(KEY, key);
                startActivity(intent);
            } else {
                // Renderable
                Intent intent = new Intent(this, FileView.class);
                intent.putExtra(FILENAME, name);
                intent.putExtra(KEY, key);
                startActivity(intent);
            }
        } catch(Exception e) {
            showError(e.toString());
        }
    }
    private String extractAsset(String assetName) throws IOException {
        File file = new File(getCacheDir(), assetName);
        if (!file.exists()) {
            InputStream asset = getAssets().open(assetName);
            FileOutputStream output = new FileOutputStream(file);
            final byte[] buffer = new byte[1024];
            int size;
            while ((size = asset.read(buffer)) != -1) {
                output.write(buffer, 0, size);
            }
            asset.close();
            output.close();
        }
        return file.getCanonicalPath();
    }
    private void viewAsset(String assetName) {
        try {
            String assetFilename = extractAsset(assetName);

            view(documentFilters().GetExtractor(assetFilename), assetName, assetName);
        } catch(Exception e) {
            showError(e.toString());
        }
    }
    private void viewSubFile(int subfileIndex) {
        try {
            FileInfo info = mFiles.get(subfileIndex);
            view(mExtractor.GetSubFile(info.id), info.name, mExtractorKey + "_" + info.id);
        } catch(Exception e) {
            showError(e.toString());
        }
    }
    private void loadAssetFileList() {
        try {
            ArrayList<String> fileList = new ArrayList<String>();
            String[] assets = getBaseContext().getAssets().list("");

            fileList.addAll(Arrays.asList(assets));

            mListAdapter = new ArrayAdapter<String>(this, R.layout.file_browser_row, fileList);
            mListView.setAdapter(mListAdapter);

            final FileBrowser self = this;
            mListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                    self.viewAsset(((TextView) view).getText().toString());
                }
            });
        } catch(Exception e) {
            showError(e.toString());
        }
    }
    private void loadDocumentFileList(Extractor document, String key) {
        this.mExtractor = document;
        this.mExtractorKey = key;

        try {
            ArrayList<String> fileList = new ArrayList<>();
            for (SubFile subfile = document.GetFirstSubFile(); subfile != null; subfile = document.GetNextSubFile()) {
                mFiles.add(new FileInfo(subfile.getName(), subfile.getID()));
                fileList.add(subfile.getName());
                subfile.Close();
            }
            mListAdapter = new ArrayAdapter<String>(this, R.layout.file_browser_row, fileList);
            mListView.setAdapter(mListAdapter);

            final FileBrowser self = this;
            mListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                    self.viewSubFile(i);
                }
            });
        } catch(Exception e) {
            showError(e.toString());
        }
    }
}
