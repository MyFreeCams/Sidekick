Set-ExecutionPolicy RemoteSigned -scope CurrentUser
(scoop install 7zip git cmake curl grep openssh sed) -or ((Invoke-Expression (New-Object System.Net.WebClient).DownloadString('https://get.scoop.sh')) -and (scoop install 7zip git cmake curl grep openssh sed))
