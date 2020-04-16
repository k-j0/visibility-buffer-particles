


// Process names
#define VBP_PROCESS "vBufferParticles"
#define MSIA_PROCESS "MSIAfterburner"





#include <iostream>
#include <thread>
#include <string>
#include <functional>
#include <filesystem>
#include <chrono>

// C++/CLI includes/usings
#include <windows.h>
#using <mscorlib.dll>
#using "System.Dll"
#using "System.Windows.Forms.Dll"
#using "System.Drawing.Dll"

using namespace System;
using namespace System::Drawing;
using namespace System::Collections;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Diagnostics;

// Renderers
#define VISIBILITY 0
#define GBUFFER3 1
#define GBUFFER6 2
#define FORWARD 3

// Geometry modes
#define VERT 0
#define GEOM 1
#define COMP 2
#define VERTGEOM 3

#define TEST_SETUP_SECONDS 5 // time spent waiting for the app to start up before starting to benchmark performance - must be > 1.5 at least
int testLengthSeconds = 30;// total time taken for each test; must be higher than TEST_SETUP_SECONDS as it is included.


int testNum = 0;
int testAmount = 272;// 272 tests in total + 640 for full particle counts + 164 for full particle sizes

std::chrono::time_point<std::chrono::steady_clock> startTime;

// Click locations as defined by the user upon start-up.
POINT afterburnerRightClick;
POINT afterburnerClearHistory;
POINT afterburnerStartLogging;



/// Returns GC pointers to all processes currently running with a certain name
array<Process^>^ getProcesses(std::string procName) {
	System::String^ pName = gcnew String(procName.c_str());
	auto processes = Process::GetProcessesByName(pName);
	if (processes->Length <= 0) {
		std::cout << "No processes with name '" << procName << "'! " << std::endl;
	}
	return processes;
}

/// Simulates a key press
// time taken: .1s
void pressKey(DWORD button) {
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press key
	ip.ki.wVk = (WORD)button;
	ip.ki.dwFlags = 0;
	SendInput(1, &ip, sizeof(INPUT));

	// Release key
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));

	Sleep(100);
}

/// Simulates a click (press + release) at a precise location
// time taken: .1s
void click(int x, int y, bool rightClick) {

	// move mouse
	SetCursorPos(x, y);
	Sleep(100);

	// prepare input
	INPUT ip;
	ip.type = INPUT_MOUSE;
	ip.mi.dx = 0;
	ip.mi.dy = 0;
	ip.mi.dwFlags = rightClick? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN;
	ip.mi.time = 0;
	ip.mi.dwExtraInfo = 0;

	// Press button
	SendInput(1, &ip, sizeof(INPUT));
	//Sleep(100);

	// Release button
	ip.mi.dwFlags = rightClick ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;
	SendInput(1, &ip, sizeof(INPUT));
	//Sleep(100);
}

/// Shorthand for above function
void click(POINT p, bool rightClick) {
	click(p.x, p.y, rightClick);
}

/// Kills all processes with the associated name
void killProcesses(std::string processName) {
	auto processes = getProcesses(processName);
	for (int i = 0; i < processes->Length; ++i) {
		processes[i]->CloseMainWindow();
		processes[i]->WaitForExit();
	}
}

/// Focuses OS on a process
void bringProcessToForeground(std::string processName) {
	auto processes = getProcesses(processName);
	for (int i = 0; i < processes->Length; ++i) {
		if (processes[i]->MainWindowHandle == IntPtr::Zero) {
			ShowWindow((HWND)processes[i]->Handle.ToPointer(), 9);// in case the window is minimized
		}
		SetForegroundWindow((HWND)processes[i]->MainWindowHandle.ToPointer());
	}
}


/// Clicks Afterburner in order to clear history
// time taken: .5s
void clearAfterburnerHistory() {
	bringProcessToForeground(MSIA_PROCESS);
	Sleep(100);
	click(afterburnerRightClick, true);
	Sleep(100);
	click(afterburnerClearHistory, false);
	Sleep(100);
}

/// Clicks Afterburner to enable or disable logs
// time taken: .5s
void toggleAfterburnerLogging() {
	bringProcessToForeground(MSIA_PROCESS);
	Sleep(100);
	click(afterburnerRightClick, true);
	Sleep(100);
	click(afterburnerStartLogging, false);
	Sleep(100);
}

/// Renames Afterburner session, assumed to be placed under Benchmarks/__.hml
void renameAfterburnerSessionFile(std::string newName) {
	rename("../vbparts-benchmarks/Benchmarks/__.hml", ("../vbparts-benchmarks/Benchmarks/"+newName+".csv").c_str());
}



/// Waits for a bit, processes frame times and performance, then kills all processes with the name "vBufferParticles"
// time taken: testLengthSeconds
void runProgram() {

	// wait for the program to open up and initialize...
	Sleep(TEST_SETUP_SECONDS * 1000 - 1500);

	// Start capturing data with Afterburner...
	clearAfterburnerHistory();
	toggleAfterburnerLogging();

	// Wait for the data to be collected...
	bringProcessToForeground(VBP_PROCESS);
	Sleep((testLengthSeconds -TEST_SETUP_SECONDS) * 1000); // wait to give the program some time to run...

	// Stop capturing data with Afterburner...
	toggleAfterburnerLogging();

	// Stop particles app
	killProcesses(VBP_PROCESS);

}

/// Starts the process vBufferParticles, and returns after stopping it a bit later.
void openProgram(int width, int height, int renderer, int pmode, float pspread, float psize, int pcount, int pcomplexity, bool cutout) {
	
	// Determine where the results will be stored
	std::string rendererName = (renderer == VISIBILITY ? "v" : renderer == GBUFFER3 ? "g3" : renderer == GBUFFER6 ? "g6" : "fwd");
	std::string pModeName = (pmode == VERT ? "ve" : pmode == GEOM ? "ge" : pmode == COMP ? "co" : "vege");
	std::string filename = rendererName + "_" + pModeName + "_" + std::to_string(pcount) + "_" + std::to_string(width) + "x" + std::to_string(height) + "_" + std::to_string(pcomplexity) + "_" + std::to_string((int)(pspread*1000.0f)) + "_" + std::to_string((int)(psize*1000.0f));
	printf(("Results will be stored to " + filename + "\n").c_str());
	
	// Skip the test if it's already been done
	if (std::filesystem::exists("../vbparts-benchmarks/Benchmarks/" + filename + ".csv")) {
		printf("File already exists. No need for this test.\n");
		return;
	}
	
	printf("---> Opening vBufferParticles...\n\n");
	
	std::thread t(runProgram);
	
	// Start the program from the command line with required arguments
	std::string command = "\"Bin\\vBufferParticles.exe\""
							" -ui:0"	// no ui
							" -width:" + std::to_string(width) +	// window resolution
							" -height:" + std::to_string(height) +		// window resolution
							" -shadercomp:0"	// no shader compilation
							" -renderer:" + rendererName +	// renderer mode
							" -pmode:" + pModeName +	// particle generation mode
							" -pspread:" + std::to_string(pspread) +	// particle spread
							" -psize:" + std::to_string(psize) +	// particle half size
							" -pcount:" + std::to_string(pcount) +	// particle count
							" -pcomplexity:" + std::to_string(pcomplexity) + // particle complexity
							" -freeze:1" +	// freeze time
							" -cutout:" + (cutout ? "1":"0");	// whether to use cutout particles
	system(command.c_str());
	
	// Wait for benchmark to be over (system() is what should stall, join() actually shouldn't block at this point if all went fine)
	t.join();
	
	// Retrieve logging file to rename it
	renameAfterburnerSessionFile(filename);
	
	printf(("\n\n---> Done recording performance. Results saved in file: "+filename+".csv\n\n").c_str());
}


/// Gets the position of the cursor after waiting a certain amount of seconds
void fetchCursor(std::string name, POINT& outP, int steps) {
	printf(("Place cursor above: " + name + ".\n").c_str());
	for (int i = 0; i <= steps; ++i) {
		Sleep(1000);
		std::cout << steps - i << "  ";
	}
	std::cout << "\n";
	GetCursorPos(&outP);
}

/// The settings available to be changed through command line parameters in the main app
struct Settings {
	int width = 1024;
	int height = 768;
	int renderer = VISIBILITY;
	int pmode = VERT;
	float spread = 0.4f;
	float size = 0.03f;
	int count = 1024 * 1024;
	int complexity = 2;
	bool cutout = false;
} settings;

/// Starts a test with a specific set of settings
void record(Settings s) {
	++testNum;

	std::cout << "> Starting test #" << testNum << "/" << testAmount << " (" << int(float(testNum) / testAmount * 100.0f) << "%)." << std::endl;
	std::chrono::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration elapsed = now - startTime;
	int minutesSpent = std::chrono::duration_cast<std::chrono::minutes>(elapsed).count();
	std::cout << "\tSpent " << minutesSpent << " mins so far; expect about " << (testLengthSeconds * testAmount / 60) << " mins total." << std::endl << std::endl;

	openProgram(s.width, s.height, s.renderer, s.pmode, s.spread, s.size, s.count, s.complexity, s.cutout);
}




/// All different tests defined here


// 4 tests
void rendererTests(Settings settings) {
	settings.renderer = VISIBILITY;
	record(settings);

	settings.renderer = GBUFFER3;
	record(settings);

	settings.renderer = GBUFFER6;
	record(settings);

	settings.renderer = FORWARD;
	record(settings);
}

// 16 tests (4 * 4)
void particleModeTests(Settings settings) {

	settings.pmode = VERT;
	rendererTests(settings);// test for each renderer

	settings.pmode = GEOM;
	rendererTests(settings);// test for each renderer

	settings.pmode = COMP;
	rendererTests(settings);// test for each renderer

	settings.pmode = VERTGEOM;
	rendererTests(settings);// test for each renderer
}

// 16 tests (1 * 16)
void noParticlesTests(Settings settings) {

	settings.count = 0;
	particleModeTests(settings);

}

// 48 tests (3 * 16)
void resolutionTests(Settings settings) {

	settings.width = 1024;
	settings.height = 768;
	particleModeTests(settings);

	settings.width = 1536;
	settings.height = 1024;
	particleModeTests(settings);

	settings.width = 768;
	settings.height = 512;
	particleModeTests(settings);

}

// 64 tests (4 * 16)
void complexityTests(Settings settings) {

	settings.complexity = 0;
	particleModeTests(settings);

	settings.complexity = 1;
	particleModeTests(settings);

	settings.complexity = 2;
	particleModeTests(settings);

	settings.complexity = 3;
	particleModeTests(settings);

}

// 48 tests (3 * 16)
void particleCountTests(Settings settings) {

	settings.count = 1024 * 1024 * 2;
	particleModeTests(settings);

	settings.count = 1024 * 1024;
	particleModeTests(settings);

	settings.count = 1024;
	particleModeTests(settings);

}

// 320 tests (20 * 16)
void fullParticleCountTests(Settings settings) {

	settings.count = 0;
	for (int i = 0; i < 20; ++i) {
		settings.count += 262144; // 2^18 increments; 2^18*20 = 5,242,880
		particleModeTests(settings);
	}

}

// 48 tests (3 * 16)
void particleSizeTests(Settings settings) {

	settings.size = 0.03f;
	particleModeTests(settings);

	settings.size = 0.3f;
	particleModeTests(settings);

	settings.size = 0.003f;
	particleModeTests(settings);

}

// 164 tests (41 * 4)
void fullParticleSizeTests(Settings settings) {

	settings.size = 0;
	settings.pmode = VERT;
	for (int i = 0; i < 41; ++i) {
		rendererTests(settings);
		settings.size = 0.03f * i; // 0.03 increments; 0.03*40 = 1.2
	}

}

// 48 tests (3 * 16)
void particleSpreadTests(Settings settings) {

	settings.spread = 0.4f;
	particleModeTests(settings);

	settings.spread = 0;
	particleModeTests(settings);

	settings.spread = 0.8f;
	particleModeTests(settings);

}

///----------------




int main(int argc, char** argv) {

	// Apply command-line params
	bool usualTests = true, fullCountTests = false, fullSizeTests = false, cutout = false;
	for (int i = 0; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg.size() > 0){
			if (arg[0] == '-') {// potentially a valid flag
				arg = arg.substr(1);
				if (arg == "no-usual") {
					usualTests = false;
					testAmount -= 272;
					std::cout << "Will bypass usual tests." << std::endl;
				} else if (arg == "full-count") {
					fullCountTests = true;
					testAmount += 320;
					std::cout << "Will execute full particle count tests." << std::endl;
				} else if (arg == "full-size") {
					fullSizeTests = true;
					testAmount += 164;
					std::cout << "Will execute full particle size tests." << std::endl;
				} else if (arg == "cutout") {
					cutout = true;
					std::cout << "All tests will be executed with cut-out mode turned on. Note that this may produce unexpected results for tests with particle complexity != 2." << std::endl;
				}
			} else if (arg[0] == '@') {// potentially the test length, in seconds.
				try {
					testLengthSeconds = std::stoi(arg.substr(1));
					if (testLengthSeconds < TEST_SETUP_SECONDS + 1) testLengthSeconds = TEST_SETUP_SECONDS + 1;
					std::cout << "Each test will run for " << testLengthSeconds << " seconds." << std::endl;
				} catch (...) {
					testLengthSeconds = 30;
					std::cout << "Error, could not deduce test length; tests will run for " << testLengthSeconds << " seconds by default." << std::endl;
				}
			}
		}
	}

	// Setup cursor positions
	// Takes approx. 17 seconds to set-up
	printf("Please follow the on-screen instructions by placing your mouse cursor at the designated locations.\n\n");
	Sleep(2000);
	fetchCursor("Afterburner right click", afterburnerRightClick, 6);
	fetchCursor("Clear history", afterburnerClearHistory, 4);
	fetchCursor("Log history to file", afterburnerStartLogging, 4);
	Sleep(1000);

	startTime = std::chrono::high_resolution_clock::now();

	// Perform tests (208 total if no extensive pCount tests are performed)
	Settings settings;
	settings.cutout = cutout;
	if (usualTests) {
		noParticlesTests(settings); // 16 tests
		resolutionTests(settings); // 48 tests
		complexityTests(settings); // 64 tests (48 effectively)
		particleCountTests(settings); // 48 tests (32 effectively)
		particleSizeTests(settings); // 48 tests (32 effectively)
		particleSpreadTests(settings); // 48 tests (32 effectively)
	}
	if (fullCountTests) {
		fullParticleCountTests(settings); // 320 tests
	}
	if (fullSizeTests) {
		fullParticleSizeTests(settings); // 320 tests
	}
}
