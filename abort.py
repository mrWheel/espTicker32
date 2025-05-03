import sys

# ======== abort if no environment selected ========================================================
# This script is used to abort the build process if no environment is selected.
# It checks if the environment variable "PIOENV" is set.
# If it is not set, it prints an error message and exits with a non-zero status code.
# This is useful for ensuring that the user selects an environment before running the build process.
# This script is called from the platformio.ini file using the "pre" option.
# The "pre" option allows you to run a script before the build process starts.
# =================================================================================================== 

print("\nERROR: No environment selected.")
print("Please choose one using:\n")
print("    pio run -e espTicker32Neopixels")
print("or:")
print("    pio run -e espTicker32Parola\n")
sys.exit(1)
