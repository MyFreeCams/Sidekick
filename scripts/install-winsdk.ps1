# $winsdk_url = "https://download.microsoft.com/download/4/2/2/42245968-6A79-4DA7-A5FB-08C0AD0AE661/windowssdk/winsdksetup.exe"
$winsdk_url = "https://download.microsoft.com/download/1/c/3/1c3d5161-d9e9-4e4b-9b43-b70fe8be268c/windowssdk/winsdksetup.exe"
Invoke-WebRequest -Uri $winsdk_url -OutFile winsdksetup.exe
# $sdk_args = "/features OptionId.WindowsPerformanceToolkit OptionId.WindowsDesktopDebuggers OptionId.SigningTools OptionId.WindowsSoftwareLogoToolkit OptionId.NetFxSoftwareDevelopmentKit /q"
$sdk_args = "/q"
Write-Information -MessageData "Installing Windows Debugging Tools, Performance Toolkit, Signing Tools..." -InformationAction Continue
Start-Process -Wait -FilePath "winsdksetup.exe" -ArgumentList $sdk_args
Write-Information -MessageData "Finished" -InformationAction Continue
Remove-Item -Force winsdksetup.exe
