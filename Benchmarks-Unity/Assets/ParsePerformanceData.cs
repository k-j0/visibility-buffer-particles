using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Unity.EditorCoroutines.Editor;

/// Given a list of CSV files expected to come from the vbparts-benchmarks tool, combines all the tests into a single large CSV file
[ExecuteInEditMode]
public class ParsePerformanceData : MonoBehaviour {

	[SerializeField] List<TextAsset> files; // input files
	[SerializeField] int headerSize = 10; // how big the header is in the input files - this depends on the GPU and amount of data types collected
	[SerializeField] List<ColumnType> columnTypes; // types of data in the input CSV files by column
	[SerializeField] string outputFile = ""; // CSV file where the results will be saved
	[SerializeField] bool execute = false; // checkbox in editor to execute the full analysis once

	CSVWriter csv; // used to output the data to a CSV file

	/// The different available data types
	[System.Serializable]
	enum DataType {
		GpuUsage,
		FbUsage,
		BusUsage,
		MemoryUsage,
		Frametime,
		DedicatedMemory,
		SharedMemory
	}// enum DataType

	// Represents one column in the input files, with a certain datatype.
	[System.Serializable]
	class ColumnType {
		public DataType dataType; // type of data in the column
		public int columnIndex;// a value of 0 here maps to the 3rd column in the CSV file, after the metadata cols.
		public bool writeTofile;// whether the values will be written out to the output CSV file at all.
		[HideInInspector] public List<float> values; // the list of values taken directly from the input files
		[HideInInspector] public float min, quartile1, median, quartile3, max, average;// the 5 quartiles + average value

		/// Resets all values in the column
		public void ResetVals() {
			values = new List<float>();
		}

		/// Setter for all quartiles at once
		public void SetQuartiles(float q0, float q1, float q2, float q3, float q4, float avg) {
			min = q0;
			quartile1 = q1;
			median = q2;
			quartile3 = q3;
			max = q4;
			average = avg;
		}

		public override string ToString() {
			return dataType + ": [ Min: " + min + ", 1st Quartile: " + quartile1 + ", Median: " + median + ", 3rd Quartile: " + quartile3 + ", Max: " + max + ", Average: " + average + " ]";
		}
	}// class ColumnType

	/// Given a list of sorted values, returns one quartile value from it
	float getQuartile(int q, List<float> sortedValues) {
		if(sortedValues.Count < 1) {
			Debug.LogError("Cannot get quartile out of empty list!");
			return 0;
		}
		if(q < 0 || q > 5) {
			Debug.LogError("Quartile " + q + " is not a thing...");
			return 0;
		}
		return sortedValues[(int)((sortedValues.Count-1) * (float)q/4)];
	}

	// Called once for each column type, to make sense of all the contained data.
	void interpretColumn(int columnTypeIndex) {
		ColumnType cType = columnTypes[columnTypeIndex];
		// find the 5 quartiles + the average
		float q0, q1, q2, q3, q4, avg;
		List<float> sortedValues = cType.values;
		sortedValues.Sort();
		q0 = getQuartile(0, sortedValues);
		q1 = getQuartile(1, sortedValues);
		q2 = getQuartile(2, sortedValues);
		q3 = getQuartile(3, sortedValues);
		q4 = getQuartile(4, sortedValues);
		avg = 0;
		foreach(float f in sortedValues) {
			avg += f;
		}
		avg /= sortedValues.Count;
		columnTypes[columnTypeIndex].SetQuartiles(q0, q1, q2, q3, q4, avg);
	}

	// Returns -1 if the frametime is either ALWAYS 0 or NEVER 0, returns the index of the first occurence of a zero frametime otherwise.
	int isFrameTime0() {
		foreach(ColumnType cType in columnTypes) {
			if(cType.dataType == DataType.Frametime) {
				int first0Index = -1;
				bool nonZeroData = false;
				for(int i = 0; i < cType.values.Count; ++i) {
					if(cType.values[i] <= 0) { // record the first instance of a 0 in the data
						if(first0Index == -1) first0Index = i;
					} else {
						nonZeroData = true;
					}
				}
				if(first0Index > -1 && nonZeroData) return first0Index;
				return -1;
			}
		}
		return -1;
	}

	// Called once before interpreting the data, to make sure everything is cleaned up.
	void cleanValues() {
		// On frames where the frame time is unusually 0, RivaTuner skipped a frame and we therefore need to clean that frame up.
		int frameTime0Index = isFrameTime0();
		if(frameTime0Index > -1) {
			foreach(ColumnType cType in columnTypes) {
				// remove this data point for all data types, and the 2 neighbouring frames.
				cType.values.RemoveAt(frameTime0Index);
				if(cType.values.Count > frameTime0Index + 1)
					cType.values.RemoveAt(frameTime0Index + 1);
				if(frameTime0Index > 0)
					cType.values.RemoveAt(frameTime0Index - 1);
			}
			cleanValues();// clean any other values
		}
	}

	// Called once for each file, to make sense of all its data as it's been read out.
	void interpretValues() {
		cleanValues();
		for(int i = 0; i < columnTypes.Count; ++i)
			interpretColumn(i);
	}

	// Get values out from single CSV line
	bool parseLine(string line) {
		// split into columns
		string[] cols = line.Split(new string[] { "," }, System.StringSplitOptions.RemoveEmptyEntries);
		// Read the data out according to the defined column types; skip the first 2 (meta) columns.
		foreach(ColumnType cType in columnTypes) {
			if(cols.Length >= cType.columnIndex + 2) {
				string entryStr = cols[cType.columnIndex + 2];
				try {
					float entry = entryStr.Substring(0, 3) == "N/A" ? 0 : float.Parse(entryStr);
					cType.values.Add(entry);
				}catch(System.Exception e) {
					Debug.LogError("Cannot parse float from string \"" + entryStr + "\".");
					return false;
				}
			} else {
				Debug.LogError("Line is not long enough to fit column type " + cType.dataType + " (index " + cType.columnIndex + "):\n" + line);
				return false;
			}
		}
		return true;
	}

	// Read all lines out of a CSV file
	bool parseFile(TextAsset file) {
		csv.InsertValue(file.name);
		// empty out the values from the previous file
		for(int i = 0; i<columnTypes.Count; ++i) {
			columnTypes[i].ResetVals();
		}
		// split into lines
		string[] lines = file.text.Split(new string[] { "\n" }, System.StringSplitOptions.RemoveEmptyEntries);
		// Skip the first 9 lines as they only contain metadata, and the first and last frame.
		for(int line = headerSize+1; line < lines.Length-1; ++line) {
			if(!parseLine(lines[line])) return false;
		}
		interpretValues();
		bool first = true;
		foreach(ColumnType cType in columnTypes) {
			if(!cType.writeTofile) continue;
			if(!first) csv.InsertValue("");
			first = false;
			// Write header to CSV
			csv.InsertValue(""+cType.dataType);
			csv.InsertValue("");
			csv.InsertValue(cType.average);
			csv.InsertValue("");
			csv.InsertValue(cType.min);
			csv.InsertValue(cType.quartile1);
			csv.InsertValue(cType.median);
			csv.InsertValue(cType.quartile3);
			csv.InsertValue(cType.max);
			csv.InsertValue("");
			// Write full data
			csv.InsertValues(cType.values);
			// move on to next column (either next type, or next file)
			csv.NextColumn();
		}
		return true;
	}

	// Read all files
	IEnumerator parseFiles() {
		csv = new CSVWriter();

		// Create first column in CSV.
		csv.InsertValue("Test name");
		csv.InsertValue("Data type");
		csv.InsertValue("");
		csv.InsertValue("Average");
		csv.InsertValue("");
		csv.InsertValue("Minimum");
		csv.InsertValue("1st Quartile");
		csv.InsertValue("Median");
		csv.InsertValue("3rd Quartile");
		csv.InsertValue("Maximum");
		csv.InsertValue("");
		csv.InsertValue("Data");
		csv.NextColumn();

		for(int i = 0; i < files.Count; ++i) {
			Debug.Log("File " + i + " of " + files.Count + " (" + (100 * i / files.Count) + "%)");
			parseFile(files[i]);
			yield return null;
		}
		csv.Write(outputFile);

		Debug.Log("Done.");
	}

	private void Update() {
		// Parse files whenever the Execute checkbox is clicked in the Unity Editor.
		if(execute) {
			execute = false;
			EditorCoroutineUtility.StartCoroutine(parseFiles(), this);
		}
	}

}
