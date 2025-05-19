# Build script for TeubeCreamer VST3 plugin

# Configuration
$SolutionPath = "Builds\VisualStudio2022\TeubeCreamer.sln"
$Configuration = "Release"
$Platform = "x64"
$VST3OutputPath = "$env:COMMONPROGRAMFILES\VST3\TeubeCreamer.vst3"

# Visual Studio and MSBuild paths
$VSInstallPath = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"
$MSBuildPath = "${VSInstallPath}\MSBuild\Current\Bin\MSBuild.exe"
$DevEnvPath = "${VSInstallPath}\Common7\IDE\devenv.exe"

# Function to check if a file exists
function Test-FileExists {
    param ($filePath)
    return (Test-Path $filePath)
}

# Check for required tools
Write-Host "Checking for required tools..."
$requiredTools = @(
    @{Name = "MSBuild"; Path = $MSBuildPath },
    @{Name = "Visual Studio"; Path = $DevEnvPath }
)

foreach ($tool in $requiredTools) {
    if (-not (Test-FileExists $tool.Path)) {
        Write-Error "Error: $($tool.Name) not found at $($tool.Path)"
        Write-Host "Please ensure Visual Studio 2022 is installed correctly."
        exit 1
    }
}

# Verify solution file exists
if (-not (Test-FileExists $SolutionPath)) {
    Write-Error "Error: Solution file not found at $SolutionPath"
    exit 1
}

# Build the solution
Write-Host "Building solution..."
try {
    # Using MSBuild for better control and output
    $buildArgs = @(
        $SolutionPath,
        "/p:Configuration=$Configuration",
        "/p:Platform=$Platform",
        "/t:Build",
        "/m",
        "/v:detailed"  # Add detailed output for better error reporting
    )
    
    Write-Host "Running MSBuild with arguments: $($buildArgs -join ' ')"
    $buildResult = & $MSBuildPath $buildArgs 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build output:"
        $buildResult | ForEach-Object { Write-Host $_ }
        throw "Build failed with exit code $LASTEXITCODE"
    }
    
    Write-Host "Build completed successfully!"
    
    # Copy VST3 to common VST3 folder
    $vst3Path = "Builds\VisualStudio2022\x64\Release\VST3\TeubeCreamer.vst3"
    if (Test-Path $vst3Path) {
        Write-Host "Copying VST3 to $VST3OutputPath..."
        Copy-Item -Path $vst3Path -Destination $VST3OutputPath -Force
        Write-Host "VST3 plugin installed successfully!"
    }
    else {
        Write-Error "Error: VST3 file not found at $vst3Path"
        exit 1
    }
}
catch {
    Write-Error "Error during build: $_"
    exit 1
} 