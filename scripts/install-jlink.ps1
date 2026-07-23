$downloadPage = "https://www.segger.com/downloads/jlink/" +
    "JLink_Windows_V952_x86_64.exe"

Write-Host "Opening the official SEGGER download page."
Write-Host "Review and accept the Terms of Use, then install the 64-bit package."
Start-Process $downloadPage

