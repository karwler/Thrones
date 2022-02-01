package org.duravia.thrones;

import android.content.res.AssetManager;

import org.libsdl.app.SDLActivity;

import java.io.IOException;

public class Thrones extends SDLActivity {

	public String[] listAssets(String path) {
		try {
			return getContext().getAssets().list(path);
		} catch (IOException e) {
			e.printStackTrace();
		}
		return new String[] {};
	}

}
