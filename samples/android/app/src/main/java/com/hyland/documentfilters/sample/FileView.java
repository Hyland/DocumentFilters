// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

package com.hyland.documentfilters.sample;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.content.Intent;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.perceptive.documentfilters.*;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;


public class FileView extends AppCompatActivity {

    private Button mButtonNext;
    private Button mButtonPrevious;
    private ImageView mImageView;
    private TextView mTextError;
    private LinearLayout mNavButtons;
    private int mPageIndex = 0;
    private String mFilename = "";
    private String mExtractorKey = "";
    private Extractor mExtractor;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_view);

        mImageView = findViewById(R.id.image);
        mButtonPrevious = findViewById(R.id.previous);
        mButtonNext = findViewById(R.id.next);
        mTextError = findViewById(R.id.textErrorMessage);
        mNavButtons= findViewById(R.id.navButtons);
        mPageIndex = 0;

        final FileView self = this;
        mButtonNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                self.nextPage();
            }
        });
        mButtonPrevious.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                self.prevPage();
            }
        });

        Intent intent = getIntent();
        mFilename = intent.getStringExtra(FileBrowser.FILENAME);
        mExtractorKey = intent.getStringExtra(FileBrowser.KEY);

        setTitle(mFilename);
    }

    @Override
    public void onStart() {
        super.onStart();
        loadDocument(mExtractorKey);
        showPage(0);
    }

    @Override
    public void onStop() {
        FileBrowser.closeExtractor(mExtractorKey, mExtractor);
        super.onStop();
    }
    protected void loadDocument(String key) {
        try {
            mExtractor = FileBrowser.getExtractor(key);
            mExtractor.Open(isys_docfiltersConstants.IGR_FORMAT_IMAGE, "CONTAINERS_LIST_DIR=on");
        } catch(Exception e) {
            mExtractor = null;
            setError(e.toString());
        }
    }
    protected void setError(String text) {
        mImageView.setImageBitmap(null);
        mTextError.setText(text);
    }
    protected void nextPage()
    {
        showPage(mPageIndex+1);
    }
    protected void prevPage()
    {
        showPage(mPageIndex-1);
    }
    protected void showPage(int index)
    {
        try {
            if (mExtractor == null || index < 0 || index >= getPageCount()) {
                return;
            }
            File file = new File(getCacheDir(), "pageview.png");

            mPageIndex = index;
            mTextError.setText("");
            Page page = mExtractor.GetPage(index);
            try {
                Canvas canvas = FileBrowser.documentFilters().MakeOutputCanvas(file.getAbsoluteFile().toString(), isys_docfiltersConstants.IGR_DEVICE_IMAGE_PNG, "");
                try {
                    canvas.RenderPage(page);
                } finally {
                    canvas.Close();
                }
            } finally {
                page.Close();
            }
            mImageView.setImageBitmap(BitmapFactory.decodeFile(file.getAbsoluteFile().toString()));
            mButtonPrevious.setEnabled(index > 0);
            mButtonNext.setEnabled(index < getPageCount()-1);

            mNavButtons.setVisibility(getPageCount() > 1 ? View.VISIBLE : View.GONE);

        }
        catch(Exception e)
        {
            setError(e.toString());
        }
    }
    public int getPageCount() {
        try {
            return mExtractor != null ? mExtractor.GetPageCount() : 0;
        } catch(Exception e) {
            return 0;
        }
    }
}
