
using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


/// Helper class to write CSV file column by column
public class CSVWriter {
	
	// Represents a single column of cells
	class Column {
		public List<string> cells = new List<string>();
	}
	List<Column> columns = new List<Column>();
	int maxColumnLength = 0;// as data is added, registers the largest column in the data set

	/// Moves on to the next column
	public void NextColumn() {
		columns.Add(new Column());
	}

	/// Inserts a value in the current cell, and moves on to the next cell
	public void InsertValue(string val) {
		if(columns.Count == 0) NextColumn();
		columns[columns.Count - 1].cells.Add(val);
		if(columns[columns.Count - 1].cells.Count > maxColumnLength) maxColumnLength = columns[columns.Count - 1].cells.Count;
	}

	/// Helper to insert a float value
	public void InsertValue(float val) {
		InsertValue(""+val);
	}

	/// Helper to insert an int value
	public void InsertValue(int val) {
		InsertValue("" + val);
	}

	/// Helper to insert a list of float values at once
	public void InsertValues(List<float> vals) {
		foreach(float val in vals)
			InsertValue(val);
	}

	/// Writes all the data out to a CSV file in the CSV/ directory, given the filename.
	public void Write(string filename) {
		StringBuilder sb = new StringBuilder();
		for(int i = 0; i < maxColumnLength; ++i) {
			for(int j = 0; j < columns.Count; ++j) {
				string val = columns[j].cells.Count > i ? columns[j].cells[i] : "";
				sb.Append(val + ",");
			}
			sb.Append("\n");
		}
		string filePath = Application.dataPath + "/CSV/" + filename + ".csv";
		StreamWriter outStream = File.CreateText(filePath);
		outStream.Write(sb);
		outStream.Close();
	}

}// class CSVWriter
