# $vs_url = "https://download.visualstudio.microsoft.com/download/pr/57d28351-e762-4ee1-aca4-16b6d3faaa33/9fd19a14823506bdbd249be8f519e4a17af494df5cd92e3dc65e110a744d6ebb/vs_Community.exe"
$vs_url = "https://download.visualstudio.microsoft.com/download/pr/a319c7ec-a0bd-4619-b966-4c58a50f7c76/978c6294fbfae59b96b2cf6914c1958fe3bf7992cad22baed25222c92390b8f2/vs_Community.exe"
Invoke-WebRequest -Uri $vs_url -OutFile vs_Community.exe
$vs_args = "--add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Component.VC.ATLMFC --includeRecommended --passive --wait"
Write-Information -MessageData "Installing Visual Studio 2019 Community" -InformationAction Continue
Start-Process -Wait -FilePath "vs_Community.exe" -ArgumentList $vs_args
Write-Information -MessageData "Finished" -InformationAction Continue
Remove-Item -Force vs_Community.exe
