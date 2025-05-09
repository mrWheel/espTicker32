import os
import shutil
import re
import glob
import time  # Added for sleep function
from datetime import datetime
from SCons.Script import DefaultEnvironment

# ==== setup platformio.ini ====
# set "program_name"
# set "program_binary" to the name of the firmware binary
# ==============================

# Get the environment
env = DefaultEnvironment()

# === Settings ===
versionsToKeep = 1  # Number of versions to keep (minimum = 1)
enableLogging = True  # Enable or disable log file
# ==================

def touch_filesystem():
    """Force rebuild of filesystem image by touching the data folder."""
    data_dir = os.path.join(env.get("PROJECT_DIR", ""), "data")

    if not os.path.exists(data_dir):
        print(f"ERROR: Data directory not found")
        return

    # Touch a .touch file in the data folder to force rebuild
    touch_file = os.path.join(data_dir, ".touch")
    
    # Creating or updating the timestamp of the .touch file
    with open(touch_file, 'a'):
        os.utime(touch_file, None)

    print(f"Touched data/.touch to force filesystem rebuild.")

def write_log(message):
    """Writes a log message to the log file."""
    if not enableLogging:
        return

    binaries_dir = os.path.join(env.get("PROJECT_DIR", ""), "binaries")
    os.makedirs(binaries_dir, exist_ok=True)
    log_file = os.path.join(binaries_dir, "log.txt")

    # Clean up paths in the message to show only filenames
    if "Copied" in message or "Deleted" in message:
        parts = message.split()
        for i, part in enumerate(parts):
            if os.path.sep in part:  # If this part contains a path
                # Remove the PROJECT_DIR part from the path
                project_dir = env.get("PROJECT_DIR", "")
                if project_dir and part.startswith(project_dir):
                    parts[i] = part[len(project_dir):].lstrip(os.path.sep)
                else:
                    parts[i] = os.path.basename(part)  # Replace with just the filename
        message = " ".join(parts)

    timestamp = datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
    
    # Check if this is a "Copied" message, which contains a destination filename
    if "Copied" in message:
        # Extract the destination filename
        parts = message.split()
        if len(parts) >= 4 and parts[1] == "firmware.bin" and parts[2] == "to":
            dest_filename = parts[3]
            
            # Read existing log file if it exists
            existing_lines = []
            if os.path.exists(log_file):
                with open(log_file, "r") as f:
                    existing_lines = f.readlines()
            
            # Filter out lines containing the same destination filename
            filtered_lines = [line for line in existing_lines if dest_filename not in line]
            
            # Write filtered lines plus new entry
            with open(log_file, "w") as f:
                f.writelines(filtered_lines)
                f.write(f"{timestamp} {message}\n")
            return
        
    # For all other messages, just append to the log file
    with open(log_file, "a") as f:
        f.write(f"{timestamp} {message}\n")

def get_version_from_cpp(program_name):
    """Extract the version from the program's .cpp file."""
    if not program_name:
        print("ERROR: No program_name provided")
        return None
        
    # Use the correct path to the source file
    src_path = os.path.join(env.get("PROJECT_DIR", ""), "src", f"{program_name}.cpp")
    if not os.path.exists(src_path):
        print(f"ERROR: File not found: src/{program_name}.cpp")
        return None

    with open(src_path, "r") as file:
        content = file.read()
    
    # Look for PROG_VERSION instead of __VERSION__
    match = re.search(r'const\s+char\*\s+PROG_VERSION\s*=\s*"([^"]+)"', content)
    if match:
        return match.group(1)
    else:
        print(f"ERROR: PROG_VERSION not found in src/{program_name}.cpp")
        return None

def parse_version(version_str):
    """Parse version string v1.2 or v1.2.3 to tuple (1,2) or (1,2,3)"""
    version_numbers = version_str.lstrip('v').split('.')
    return tuple(int(part) for part in version_numbers)

def extract_version_from_filename(filename, program_name, postfix):
    """Extract version from filename like <program>_<postfix>_v1.2.3.bin or <program>_v1.2.bin"""
    basename = os.path.basename(filename)
    if postfix:
        pattern = rf"{program_name}_{postfix}_(v\d+(?:\.\d+){{1,2}})\.bin"
    else:
        pattern = rf"{program_name}_(v\d+(?:\.\d+){{1,2}})\.bin"
    match = re.match(pattern, basename)
    if match:
        return match.group(1)
    else:
        return None

def get_all_existing_versions(binaries_dir, program_name, postfix):
    """Get all existing versions"""
    if postfix:
        pattern = os.path.join(binaries_dir, f"{program_name}_{postfix}_v*.bin")
    else:
        pattern = os.path.join(binaries_dir, f"{program_name}_v*.bin")
    matching_files = glob.glob(pattern)

    files_with_versions = []
    for file in matching_files:
        ver_str = extract_version_from_filename(file, program_name, postfix)
        if ver_str:
            files_with_versions.append((parse_version(ver_str), file))
    return files_with_versions

def manage_binaries(source_file, program_name, version_str, postfix):
    """General function to manage binaries"""
    project_dir = env.get("PROJECT_DIR", "")
    
    # Remove project_dir from paths for cleaner output
    source_rel_path = source_file
    if project_dir and source_file.startswith(project_dir):
        source_rel_path = source_file[len(project_dir):].lstrip(os.path.sep)
    
    print(f"DEBUG: manage_binaries called with source_file: {source_rel_path}, program_name: {program_name}, version: {version_str}, postfix: {postfix}")
    
    # Add retry mechanism to wait for the file to be created
    max_retries = 10  # Maximum number of retry attempts
    retry_delay = 2   # Seconds to wait between retries
    
    for attempt in range(max_retries):
        if os.path.exists(source_file):
            print(f"DEBUG: Source file found on attempt {attempt+1}: {source_rel_path}")
            break
        else:
            print(f"DEBUG: Source file not found on attempt {attempt+1}, waiting {retry_delay} seconds...")
            time.sleep(retry_delay)
    
    if not os.path.exists(source_file):
        print(f"WARNING: Source file not found after {max_retries} attempts: {source_rel_path}")
        print(f"This is normal during the first build or if the build hasn't completed yet.")
        return

    binaries_dir = os.path.join(project_dir, "binaries")
    print(f"DEBUG: Binaries directory: binaries")
    os.makedirs(binaries_dir, exist_ok=True)

    new_version = parse_version(version_str)
    print(f"DEBUG: Parsed version: {new_version}")
    
    files_with_versions = get_all_existing_versions(binaries_dir, program_name, postfix)
    print(f"DEBUG: Found {len(files_with_versions)} existing versions")

    # Check if exact same version already exists
    existing_file_same_version = None
    for ver_tuple, file in files_with_versions:
        file_rel_path = file
        if project_dir and file.startswith(project_dir):
            file_rel_path = file[len(project_dir):].lstrip(os.path.sep)
        
        print(f"DEBUG: Checking existing version: {ver_tuple} at {file_rel_path}")
        if ver_tuple == new_version:
            existing_file_same_version = file
            print(f"DEBUG: Found existing file with same version: {file_rel_path}")
            break

    if existing_file_same_version:
        try:
            os.remove(existing_file_same_version)
            existing_rel_path = existing_file_same_version
            if project_dir and existing_file_same_version.startswith(project_dir):
                existing_rel_path = existing_file_same_version[len(project_dir):].lstrip(os.path.sep)
            
            print(f"DEBUG: Deleted existing file with same version: {existing_rel_path}")
            ##write_log(f"Deleted existing file: {existing_rel_path}")
        except Exception as e:
            print(f"ERROR deleting existing file: {e}")
            ##write_log(f"ERROR deleting file: {existing_rel_path} - {e}")

    # Refresh file list after delete
    files_with_versions = get_all_existing_versions(binaries_dir, program_name, postfix)

    if postfix:
        new_filename = f"{program_name}_{postfix}_{version_str}.bin"
    else:
        new_filename = f"{program_name}_{version_str}.bin"
    print(f"DEBUG: New filename will be: {new_filename}")

    dest_path = os.path.join(binaries_dir, new_filename)
    dest_rel_path = f"binaries/{new_filename}"
    print(f"DEBUG: Destination path: {dest_rel_path}")

    try:
        shutil.copy(source_file, dest_path)
        print(f"DEBUG: Successfully copied {source_rel_path} to {dest_rel_path}")
        write_log(f"Copied {source_rel_path} to {dest_rel_path}")
    except Exception as e:
        print(f"ERROR copying file: {e}")
        write_log(f"ERROR copying file: {source_rel_path} to {dest_rel_path} - {e}")
        return

    # Version management
    files_with_versions.append((new_version, dest_path))
    files_with_versions.sort(key=lambda x: x[0], reverse=True)

    keep = max(1, versionsToKeep)  # Always keep at least 1
    print(f"DEBUG: Keeping {keep} most recent versions")
    files_to_keep = [file for (_, file) in files_with_versions[:keep]]
    files_to_delete = [file for (_, file) in files_with_versions[keep:]]
    
    # Convert to relative paths for display
    files_to_keep_rel = []
    for file in files_to_keep:
        if project_dir and file.startswith(project_dir):
            files_to_keep_rel.append(file[len(project_dir):].lstrip(os.path.sep))
        else:
            files_to_keep_rel.append(os.path.basename(file))
    
    files_to_delete_rel = []
    for file in files_to_delete:
        if project_dir and file.startswith(project_dir):
            files_to_delete_rel.append(file[len(project_dir):].lstrip(os.path.sep))
        else:
            files_to_delete_rel.append(os.path.basename(file))
    
    print(f"DEBUG: Files to keep: {files_to_keep_rel}")
    print(f"DEBUG: Files to delete: {files_to_delete_rel}")

    for old_file in files_to_delete:
        try:
            os.remove(old_file)
            old_rel_path = old_file
            if project_dir and old_file.startswith(project_dir):
                old_rel_path = old_file[len(project_dir):].lstrip(os.path.sep)
            
            print(f"DEBUG: Deleted old file: {old_rel_path}")
            ##write_log(f"Deleted old file: {old_rel_path}")
        except Exception as e:
            print(f"ERROR deleting old file: {e}")
            ##write_log(f"ERROR deleting file: {old_rel_path} - {e}")

def handle_filesystem(source, target, env):
    """Handles filesystem binary operations after build."""
    print("DEBUG: handle_filesystem called")
    
    # Get project directory for path shortening
    project_dir = env.get("PROJECT_DIR", "")
    
    # Convert source and target to strings for display
    source_str = str(source)
    target_str = str(target)
    
    # Remove project_dir from paths for cleaner output
    if project_dir and source_str.startswith(project_dir):
        source_str = source_str[len(project_dir):].lstrip(os.path.sep)
    if project_dir and target_str.startswith(project_dir):
        target_str = target_str[len(project_dir):].lstrip(os.path.sep)
    
    print(f"DEBUG: source: {source_str}")
    print(f"DEBUG: target: {target_str}")
    
    program_name = env.GetProjectOption("program_name")
    print(f"DEBUG: program_name: {program_name}")
    
    # Fix the build directory path - don't append program_name
    build_dir = env.subst("$BUILD_DIR")
    build_rel_dir = build_dir
    if project_dir and build_dir.startswith(project_dir):
        build_rel_dir = build_dir[len(project_dir):].lstrip(os.path.sep)
    
    print(f"DEBUG: Build directory: {build_rel_dir}")
    
    # Determine the filesystem type from the project options
    fs_type = env.GetProjectOption("board_build.filesystem", "littlefs")
    print(f"DEBUG: Filesystem type: {fs_type}")
    
    # Look for the filesystem binary
    filesys_bin = os.path.join(build_dir, f"{fs_type}.bin")
    filesys_rel_bin = os.path.join(build_rel_dir, f"{fs_type}.bin")
    print(f"DEBUG: Looking for filesystem binary at: {filesys_rel_bin}")
    
    if not os.path.isfile(filesys_bin):
        print(f"WARNING: Filesystem binary not found at {filesys_rel_bin}")
        print("This is normal if the filesystem hasn't been built yet.")
        return
    else:
        print(f"DEBUG: Found filesystem binary: {filesys_rel_bin}")

    version_str = get_version_from_cpp(program_name)
    print(f"DEBUG: Version string from cpp: {version_str}")

    if program_name and version_str:
        print(f"DEBUG: Managing binaries with program_name: {program_name}, version: {version_str}, postfix: FS")
        manage_binaries(filesys_bin, program_name, version_str, "FS")
    else:
        print(f"DEBUG: Skipping binary management due to missing program_name or version_str")

def handle_firmware(source, target, env):
    """Handles firmware binary operations after upload."""
    print("DEBUG: handle_firmware called")
    
    # Get project directory for path shortening
    project_dir = env.get("PROJECT_DIR", "")
    
    # Convert source and target to strings for display
    source_str = str(source)
    target_str = str(target)
    
    # Remove project_dir from paths for cleaner output
    if project_dir and source_str.startswith(project_dir):
        source_str = source_str[len(project_dir):].lstrip(os.path.sep)
    if project_dir and target_str.startswith(project_dir):
        target_str = target_str[len(project_dir):].lstrip(os.path.sep)
    
    print(f"DEBUG: source: {source_str}")
    print(f"DEBUG: target: {target_str}")
    
    program_name = env.GetProjectOption("program_name")
    print(f"DEBUG: program_name: {program_name}")
    
    # Fix the build directory path - don't append program_name
    build_dir = env.subst("$BUILD_DIR")
    build_rel_dir = build_dir
    if project_dir and build_dir.startswith(project_dir):
        build_rel_dir = build_dir[len(project_dir):].lstrip(os.path.sep)
    
    print(f"DEBUG: Build directory: {build_rel_dir}")
    
    firmware_bin = os.path.join(build_dir, "firmware.bin")
    firmware_rel_bin = os.path.join(build_rel_dir, "firmware.bin")
    print(f"DEBUG: Looking for firmware binary at: {firmware_rel_bin}")
    
    version_str = get_version_from_cpp(program_name)
    print(f"DEBUG: Version string from cpp: {version_str}")
    
    if program_name and version_str:
        program_binary = env.GetProjectOption("program_binary", program_name)
        print(f"DEBUG: program_binary (for firmware output): {program_binary}")
        manage_binaries(firmware_bin, program_binary, version_str, None)
    else:
        print(f"DEBUG: Skipping binary management due to missing program_name or version_str")

# Pre-build action: Touch filesystem to force rebuild
touch_filesystem()

# Register post-build actions
def register_post_actions(env):
    # Only register these actions if we're not in a clean operation
    if not env.GetOption('clean'):
        print("DEBUG: Registering post-build actions")
        
        # For firmware
        build_dir = env.subst("$BUILD_DIR")
        progname = env.subst("${PROGNAME}")
        
        # Remove project_dir from paths for cleaner output
        project_dir = env.get("PROJECT_DIR", "")
        build_rel_dir = build_dir
        if project_dir and build_dir.startswith(project_dir):
            build_rel_dir = build_dir[len(project_dir):].lstrip(os.path.sep)
        
        print(f"DEBUG: BUILD_DIR: {build_rel_dir}")
        print(f"DEBUG: PROGNAME: {progname}")
        
        # CHANGE: Add post-action to the firmware.bin file directly instead of the .elf file
        # This ensures the action runs after the binary is created
        env.AddPostAction("$BUILD_DIR/firmware.bin", handle_firmware)
        env.AddPostAction("upload", handle_firmware)
        
        # For filesystem - only add if the project has filesystem support
        fs_type = env.GetProjectOption("board_build.filesystem", "")
        if fs_type:  # Only add filesystem post-action if filesystem type is defined
            fs_bin_path = f"$BUILD_DIR/{fs_type}.bin"
            fs_rel_bin_path = os.path.join(build_rel_dir, f"{fs_type}.bin")
            print(f"DEBUG: Adding post-action for filesystem binary: {fs_rel_bin_path}")
            env.AddPostAction(fs_bin_path, handle_filesystem)
        else:
            print("DEBUG: Skipping filesystem post-action (no filesystem type defined)")
        
        print("DEBUG: Post-build actions registered successfully")
    else:
        print("DEBUG: Skipping post-build actions registration due to clean operation")

register_post_actions(env)
