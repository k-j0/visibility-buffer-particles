using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Unity.EditorCoroutines.Editor;

/// Used to compare frames from the main application and combine the results into a set comprehensive data points
[ExecuteInEditMode]
public class TextureComparison : MonoBehaviour{

	public List<Texture2D> textures;// list of textures to compare
	public bool compare = false;

    void Update(){
		if(compare) {
			compare = false;

			if(textures.Count < 2) {// not enough textures to compare
				Debug.LogWarning("Not enough textures to compare.");
				return;
			}

			EditorCoroutineUtility.StartCoroutine(comparisonRoutine(), this);

		}
    }

	/// Compare all textures 2 by 2 as a coroutine
	IEnumerator comparisonRoutine() {
		int id = 0;
		for(int i = 0; i < textures.Count; ++i) {
			for(int j = 0; j < i; ++j) {
				Compare(textures[i], textures[j], id++);
				yield return null;// wait one frame
			}
		}
	}

	// Compare 2 textures to each other
	void Compare(Texture2D a, Texture2D b, int id) {

		if(a.width != b.width || a.height != b.height) {
			Debug.LogWarning("Textures are not the same size...");
			return;
		}

		// compare pixel by pixel
		float totalError = 0;
		int erredComponents = 0;
		int erredPixels = 0;
		for(int x = 0; x < a.width; ++x) {
			for(int y = 0; y < a.height; ++y) {

				// read component difference
				Color cA = a.GetPixel(x, y);
				Color cB = b.GetPixel(x, y);
				float errR = Mathf.Abs(cA.r - cB.r);
				float errG = Mathf.Abs(cA.g - cB.g);
				float errB = Mathf.Abs(cA.b - cB.b);

				totalError += errR + errG + errB;
				if(errR > 0) ++erredComponents;
				if(errG > 0) ++erredComponents;
				if(errB > 0) ++erredComponents;
				if(errR + errG + errB > 0) ++erredPixels;
			}
		}

		if(totalError > 0)
			Debug.Log("(" + id + ") Error between [" + a.name + "] and [" + b.name + "]: " + totalError + " (Erred components: " + erredComponents + "/" + (a.width * a.height * 3) + " ("+((float)erredComponents/a.width/a.height/3.0f*100)+"%); Erred texels: " + erredPixels + "/" + (a.width * a.height)+ " (" + ((float)erredPixels / a.width / a.height * 100) + "%)).");
		else
			Debug.Log("(" + id + ") [" + a.name + "] and [" + b.name + "] have the same data."); // same exact data.

	}

}// class TextureComparison
