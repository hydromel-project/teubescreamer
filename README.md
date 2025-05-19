# TeubeCreamer VST3 Plugin

TeubeCreamer is a VST3 audio plugin emulating a TubeScreamer TS-1 style overdrive pedal.
It is built using the JUCE framework.

## Project Information

*   **Company:** Hydromel
*   **Product:** TeubeCreamer
*   **Plugin Format:** VST3
*   **Category:** Distortion

## Prerequisites

Before you begin, ensure you have the following installed:

1.  **JUCE Framework:** Download and install the JUCE framework from [juce.com](https://juce.com/). Make sure Projucer (the JUCE project management tool) is accessible.
2.  **C++ Compiler:** Visual Studio (e.g., Visual Studio 2022 Community Edition with C++ desktop development workload).
3.  **Git:** For cloning the repository.

## Setup and Build Instructions

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/hydromel-project/teubescreamer
    cd TeubeCreamer
    ```

2.  **Locate Projucer:**
    Make a note of the path to your Projucer application. The build script (`build.ps1` for Windows) might need this, or you might run Projucer manually.

3.  **Generate Project Files with Projucer:**
    *   Open the `AudioPluginDemo.jucer` file located in the root of the project with Projucer.
    *   In Projucer, ensure your "Visual Studio 2022" exporter is configured correctly with your JUCE module paths and SDKs if necessary.
    *   Click the "Save Project and Open in IDE" button in Projucer. This will:
        *   Generate the `JuceLibraryCode/` directory.
        *   Generate the necessary Visual Studio Solution file in the `Builds/VisualStudio2022/` directory.

    Alternatively, you can use the Projucer command-line interface if you have it set up:
    ```bash
    # Example:
    # path/to/your/Projucer.exe --resave AudioPluginDemo.jucer
    ```

4.  **Build the Plugin:**

    *   **Using the IDE (Recommended for development):**
        *   Open the generated solution file (`Builds/VisualStudio2022/AudioPluginDemo.sln`) in Visual Studio.
        *   Select the "Release" configuration.
        *   Build the project. The VST3 plugin (`TeubeCreamer.vst3`) will typically be created in `Builds/VisualStudio2022/x64/Release/VST3/`.

    *   **Using the Build Script (PowerShell):**
        A `build.ps1` script is provided for convenience.
        *   **Important:** You might need to edit `build.ps1` to set the correct paths for MSBuild (`$MSBuildPath`) and Projucer (`$ProjucerPath`) if they are not found automatically or if you have non-standard installations.
        ```powershell
        .\build.ps1
        ```
        This script will:
        1.  Attempt to re-save the `.jucer` project using Projucer (if `$ProjucerPath` is set).
        2.  Build the "Release" configuration using MSBuild.
        3.  Copy the compiled `TeubeCreamer.vst3` to the common VST3 plugins folder (`C:\Program Files\Common Files\VST3\`).

    *   **Using MSBuild directly:**
        If you have MSBuild in your PATH or know its location:
        ```powershell
        # Navigate to the project root
        # & "path\to\MSBuild.exe" .\Builds\VisualStudio2022\AudioPluginDemo.sln /p:Configuration=Release /p:Platform=x64
        ```

5.  **Locate the VST3 Plugin:**
    After a successful build, the `TeubeCreamer.vst3` bundle will be located in the build output directory (e.g., `Builds/VisualStudio2022/x64/Release/VST3/TeubeCreamer.vst3`).

6.  **Test in a DAW:**
    Copy the `TeubeCreamer.vst3` file to your VST3 plugin directory: `C:\Program Files\Common Files\VST3`
    Launch your Digital Audio Workstation (DAW), rescan for plugins if necessary, and load TeubeCreamer.

## Contributing

[Details about contributing, coding standards, pull request process, etc., can be added here later.]

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details. 