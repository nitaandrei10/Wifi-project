param (
    [string]$Port = "COM5",
    [string]$Baud = "115200"
)
python -m serial.tools.miniterm $Port $Baud
