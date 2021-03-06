﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Unity.EditorCoroutines.Editor;

/// Prettifier for the data created by ParsePerformanceData class, used to combine the data for several GPUs and only keeping relevant tests
[ExecuteInEditMode]
public class PerformanceResults : MonoBehaviour{

	public bool reset = false;// click checkbox in inspector to reset settings

	public List<TextAsset> files;// all intermediate CSV files generated by ParsePerformanceData

	/// The different settings, to select what data goes into the output CSV
	public DataType dataType = DataType.Frametime;
	public Renderer renderer = Renderer.All;
	public Mode mode = Mode.All;
	public OptionalInt particleCount;
	public OptionalInt resolutionWidth;
	public OptionalInt resolutionHeight;
	public OptionalInt particleComplexity;
	public OptionalInt particleDensity;
	public OptionalInt particleSpread;

	public string filename = "";// output filename
	public bool parse = false;// click checkbox in inspector to run thru files provided



	CSVWriter csv;



	// The different types of data, renderers and geometry modes.
	public enum DataType { None, Frametime, GpuUsage, MemoryUsage, SharedMemory, BusUsage, DedicatedMemory, FbUsage }
	public enum Renderer { None, All, Forward, GBuffer3, GBuffer6, VBuffer }
	public enum Mode { None, All, CompComp, GeomGeom, VertVert, VertGeom }
	
	// Represents an int that can hold an additional value "Unset"
	[System.Serializable] public struct OptionalInt {
		public int value;
		public bool includeAll;

		// Returns true if the value passes the test set by the optional int.
		public bool Conforms(int val) {
			return val >= 0 && (includeAll || value == val);
		}
	}// struct OptionalInt

	// Parses a data type string and returns the corresponding DataType
	DataType dataTypeFromStr(string str) {
		switch(str.ToLower()) {
			case "frametime": return DataType.Frametime;
			case "gpuusage": return DataType.GpuUsage;
			case "memoryusage": return DataType.MemoryUsage;
			case "sharedmemory": return DataType.SharedMemory;
			case "bususage": return DataType.BusUsage;
			case "dedicatedmemory": return DataType.DedicatedMemory;
			case "fbusage": return DataType.FbUsage;
			default: Debug.LogError("Invalid data type: " + str); return DataType.None;
		}
	}

	// Parses a string in the format fwd_co_0_1024x768_2_400_29 (R_M_P_WxH_C_D_S) to return the settings
	(Renderer renderer, Mode mode, int pCount, int resW, int resH, int complexity, int density, int spread) getTestParams(string testName) {
		string[] split = testName.Split('_');
		if(split.Length != 7) {
			Debug.LogError("Cannot parse " + testName + ": splitting results in " + split.Length + " strings instead of 7.");
			return (Renderer.None, Mode.None, -1, -1, -1, -1, -1, -1);
		}
		Renderer renderer;
		Mode mode;
		int pCount, resW, resH, complexity, density, size;

		switch(split[0]) {// Renderer
			case "fwd": renderer = Renderer.Forward; break;
			case "g3": renderer = Renderer.GBuffer3; break;
			case "g6": renderer = Renderer.GBuffer6; break;
			case "v": renderer = Renderer.VBuffer; break;
			default: Debug.LogError("Renderer '" + split[0] + "' is not an option..."); renderer = Renderer.None; break;
		}
		switch(split[1]) {// Gen Mode
			case "co": mode = Mode.CompComp; break;
			case "ge": mode = Mode.GeomGeom; break;
			case "ve": mode = Mode.VertVert; break;
			case "vege": mode = Mode.VertGeom; break;
			default: Debug.LogError("Mode '" + split[1] + "' is not an option..."); mode = Mode.None; break;
		}
		if(!int.TryParse(split[2], out pCount) || pCount < 0) {// Particle count
			Debug.LogError("Particle count cannot be interpreted correctly from '" + split[2] + "'...");
			pCount = -1;
		}
		string[] resSplit = split[3].Split('x');// Resolution
		if(resSplit.Length != 2) {
			Debug.LogError("Invalid resolution '" + split[3] + "': should have exactly one 'x' between width and height.");
			resW = resH = -1;
		} else {
			if(!int.TryParse(resSplit[0], out resW) || resW < 0) {
				Debug.LogError("Invalid resolution width '" + resSplit[0] + "'...");
				resW = -1;
			}
			if(!int.TryParse(resSplit[1], out resH) || resH < 0) {
				Debug.LogError("Invalid resolution height '" + resSplit[1] + "'...");
				resH = -1;
			}
		}
		if(!int.TryParse(split[4], out complexity)) {// Particle complexity
			Debug.LogError("Invalid complexity '" + split[4] + "'...");
			complexity = -1;
		}
		if(!int.TryParse(split[5], out density)) {// Particle density/spread
			Debug.LogError("Invalid density/spread '" + split[5] + "'...");
			density = -1;
		}
		if(!int.TryParse(split[6], out size)) {// Particle half size
			Debug.LogError("Invalid size '" + split[6] + "'...");
			size = -1;
		}

		return (renderer, mode, pCount, resW, resH, complexity, density, size);
	}

	/// Parses a single test by inspecting the test's settings and values
	void ParseTest(List<List<string>> contents, int col) {

		if(contents.Count < 12) {
			Debug.LogError("Less than 12 rows: cannot parse test.");
			return;
		}

		// The average can be found on 4th row
		float average = 0;
		if(!float.TryParse(contents[3][col], out average)) {
			Debug.LogError("Cannot parse average from '" + contents[3][col] + "'...");
			return;
		}

		// Rows 6..10 are min/1st q/2nd q/3rd q/max
		float min = 0, q1 = 0, q2 = 0, q3 = 0, max = 0;
		if(!float.TryParse(contents[5][col], out min)) {
			Debug.LogError("Cannot parse minimum from '" + contents[5][col] + "'...");
			return;
		}
		if(!float.TryParse(contents[6][col], out q1)) {
			Debug.LogError("Cannot parse quartile 1 from '" + contents[6][col] + "'...");
			return;
		}
		if(!float.TryParse(contents[7][col], out q2)) {
			Debug.LogError("Cannot parse median from '" + contents[7][col] + "'...");
			return;
		}
		if(!float.TryParse(contents[8][col], out q3)) {
			Debug.LogError("Cannot parse quartile 3 from '" + contents[8][col] + "'...");
			return;
		}
		if(!float.TryParse(contents[9][col], out max)) {
			Debug.LogError("Cannot parse maximum from '" + contents[9][col] + "'...");
			return;
		}

		// Ok - write average and quartiles
		csv.InsertValue(average);
		csv.InsertValue("");
		csv.InsertValue(min);
		csv.InsertValue(q1);
		csv.InsertValue(q2);
		csv.InsertValue(q3);
		csv.InsertValue(max);
		csv.InsertValue("");

		// Write full data from test.
		for(int i = 11; i < contents.Count; ++i) {
			if(contents[i].Count > col) {
				string cell = contents[i][col];
				if(cell == "") break;
				float nextVal = 0;
				if(float.TryParse(cell, out nextVal)) {
					csv.InsertValue(nextVal);
				}
			} else break;
		}

	}

	/// Parses a single CSV file ie data for one GPU
	void ParseFile(TextAsset file) {

		// Should we parse this file?
		string name = file.name;
		string gpuName = "";
		string[] splitName = name.Split('-');
		for(int i = 0; i < splitName.Length - 1; ++i) {
			gpuName += splitName[i] + " ";
		}
		gpuName = gpuName.Substring(0, gpuName.Length - 1);
		DataType dataType = dataTypeFromStr(splitName[splitName.Length - 1]);
		Debug.Log("<b>GPU Name: " + gpuName + " | Data type: " + dataType + (dataType == this.dataType? "" : " (skipping)") + "</b>");
		if(dataType != this.dataType) return;

		// Parse CSV contents into contents 2D list
		List<List<string>> contents = new List<List<string>>(); // access by [row][col], up to (rows,cols)
		int rows;
		int cols = 0;
		{
			string[] lines = file.text.Split(new char[] { '\n' }, System.StringSplitOptions.None);
			rows = lines.Length;
			for(int row = 0; row < lines.Length; ++row) {
				string[] cells = lines[row].Split(new char[] { ',' }, System.StringSplitOptions.None);
				contents.Add(new List<string>());
				if(cols < cells.Length) cols = cells.Length;
				for(int col = 0; col < cells.Length; ++col) {
					contents[row].Add(cells[col]);
				}
			}
		}

		// Only write header once for each value repeated over a number of cols
		bool writeGpuName = true;
		Renderer lastRendererWritten = Renderer.None;
		Mode lastModeWritten = Mode.None;
		
		// Go over each column
		int includedTests = 0;
		int excludedTests = 0;
		for(int col = 0; col < cols; ++col) {
			if(contents.Count > 0 && contents[0].Count > col) {
				string header = contents[0][col];
				if(header != null && header != "" && header != "Test name") {

					// Get the settings for this test
					(Renderer renderer, Mode mode, int pCount, int resW, int resH, int complexity, int density, int spread) = getTestParams(header);

					++excludedTests;

					// Is it the right renderer for the current tests?
					if(renderer == this.renderer || (this.renderer == Renderer.All && renderer != Renderer.None)) {
						// Is it the right mode for the current tests?
						if(mode == this.mode || (this.mode == Mode.All && mode != Mode.None)) {
							// Does it pass all optionalint tests?
							if(particleCount.Conforms(pCount) && resolutionWidth.Conforms(resW) && resolutionHeight.Conforms(resH) && particleComplexity.Conforms(complexity) &&
										particleDensity.Conforms(density) && particleSpread.Conforms(spread)) {

								++includedTests;
								--excludedTests;

								Debug.Log("Including test " + header + ": " + renderer + ", " + mode + ", " + pCount + ", " + resW + "x" + resH + ", " + complexity + ", " + density + ", " + spread + ".");
								
								// Write header
								csv.NextColumn();
								csv.InsertValue(writeGpuName ? gpuName : ""); writeGpuName = false;
								if(this.renderer == Renderer.All) { csv.InsertValue(lastRendererWritten != renderer ? renderer + "" : ""); lastRendererWritten = renderer; }
								if(this.mode == Mode.All) { csv.InsertValue(lastModeWritten != mode ? mode + "" : ""); lastModeWritten = mode; }
								if(this.particleCount.includeAll) { csv.InsertValue(pCount + ""); }
								if(this.resolutionWidth.includeAll) { csv.InsertValue(resW + ""); }
								if(this.resolutionHeight.includeAll) { csv.InsertValue(resH + ""); }
								if(this.particleComplexity.includeAll) { csv.InsertValue(complexity + ""); }
								if(this.particleDensity.includeAll) { csv.InsertValue(density + ""); }
								if(this.particleSpread.includeAll) { csv.InsertValue(spread + ""); }
								csv.InsertValue("");

								ParseTest(contents, col);

							}
						}
					}

				}
			}
		}

		Debug.Log("Included " + includedTests + " tests out of " + (excludedTests + includedTests) + " in file " + file.name);
	}

	/// Parses all files provided, as a coroutine.
	IEnumerator ParseAll() {
		csv = new CSVWriter();
		
		// Write constant settings to the file.
		csv.InsertValue("Constant Settings:");
		csv.InsertValue("Data Type");
		csv.InsertValue("Renderer");
		csv.InsertValue("Mode");
		csv.InsertValue("Particle Count");
		csv.InsertValue("Resolution Width");
		csv.InsertValue("Resolution Height");
		csv.InsertValue("Particle Complexity");
		csv.InsertValue("Particle Density (x1000)");
		csv.InsertValue("Particle Spread (x1000)");
		csv.NextColumn();
		csv.InsertValue("");
		csv.InsertValue(""+dataType);
		csv.InsertValue((renderer == Renderer.All) ? "[Variable]" : "" + renderer);
		csv.InsertValue((mode == Mode.All) ? "[Variable]" : "" + mode);
		csv.InsertValue(particleCount.includeAll ? "[Variable]" : "" + particleCount.value);
		csv.InsertValue(resolutionWidth.includeAll ? "[Variable]" : "" + resolutionWidth.value);
		csv.InsertValue(resolutionHeight.includeAll ? "[Variable]" : "" + resolutionHeight.value);
		csv.InsertValue(particleComplexity.includeAll ? "[Variable]" : "" + particleComplexity.value);
		csv.InsertValue(particleDensity.includeAll ? "[Variable]" : "" + particleDensity.value);
		csv.InsertValue(particleSpread.includeAll ? "[Variable]" : "" + particleSpread.value);
		csv.NextColumn();
		csv.NextColumn();

		// Write labels for each row
		csv.InsertValue("GPU");
		if(renderer == Renderer.All) csv.InsertValue("Renderer");
		if(mode == Mode.All) csv.InsertValue("Mode");
		if(particleCount.includeAll) csv.InsertValue("Particle count");
		if(resolutionWidth.includeAll) csv.InsertValue("Resolution width");
		if(resolutionHeight.includeAll) csv.InsertValue("Resolution height");
		if(particleComplexity.includeAll) csv.InsertValue("Particle complexity");
		if(particleDensity.includeAll) csv.InsertValue("Particle density");
		if(particleSpread.includeAll) csv.InsertValue("Particle spread");
		csv.InsertValue("");
		csv.InsertValue("Average");
		csv.InsertValue("");
		csv.InsertValue("Minimum");
		csv.InsertValue("1st Quartile");
		csv.InsertValue("Median");
		csv.InsertValue("2nd Quartile");
		csv.InsertValue("Maximum");
		csv.InsertValue("");
		csv.InsertValue("Full data");

		foreach(TextAsset file in files) {
			ParseFile(file);
			yield return null;
		}
		string fname = dataType + "-" + filename;
		csv.Write("results/" + fname);
		Debug.Log("Done! Results saved to " + fname);
	}

	/// Resets all editor settings to their default values
	private void Reset() {
		dataType = DataType.Frametime;
		renderer = Renderer.All;
		mode = Mode.All;
		particleCount.includeAll = false;
		particleCount.value = 1048576;
		resolutionWidth.includeAll = false;
		resolutionWidth.value = 1024;
		resolutionHeight.includeAll = false;
		resolutionHeight.value = 768;
		particleComplexity.includeAll = false;
		particleComplexity.value = 2;
		particleDensity.includeAll = false;
		particleDensity.value = 400;
		particleSpread.includeAll = false;
		particleSpread.value = 29;
	}

	private void Update() {
		if(parse) {
			parse = false;
			EditorCoroutineUtility.StartCoroutine(ParseAll(), this);
		}
		if(reset) {
			reset = false;
			Reset();
		}
	}

}// class PerformanceResults
