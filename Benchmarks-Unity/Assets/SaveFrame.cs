using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// Helper class used to take a screenshot and save it as an image file
public class SaveFrame : MonoBehaviour{

	public string filename = "screenshot";

	IEnumerator Start() {
		yield return null;// wait until the first frame is rendered
		ScreenCapture.CaptureScreenshot(filename + "_" + DateTime.Now.Ticks + ".png");
	}

}// class SaveFrame
