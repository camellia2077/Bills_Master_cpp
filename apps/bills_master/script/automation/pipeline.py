import sys
import time
from pathlib import Path
from .utils import run_command, setup_environment
from .cmake import ensure_cmake_configured, build_target

# We need to access config.py from the parent directory
# Since this is a package, we'll expect the entry point (manage.py) to handle sys.path or import config
# But to be robust, we can try to import it if sys.path allows, or pass it in.
# Let's assume the callers (run_* functions) will receive config values.

def handle_clean(build_dir: Path, generator: str, clean_flag: bool):
    """Clean the build directory if requested."""
    if clean_flag and build_dir.is_dir():
        print(f"==> 'clean' option provided. Cleaning existing build directory...")

        if generator == "Ninja":
            clean_cmd = ["ninja", "clean"]
        elif generator == "Unix Makefiles":
            clean_cmd = ["make", "clean"]
        else:
             # Fallback or error
             print(f"!!! Warning: Unknown generator '{generator}', skipping specific clean command.")
             return

        run_command(clean_cmd, cwd=build_dir)
        print(f"==> Build directory '{build_dir.name}' cleaned.")

def run_build(build_type: str, build_dir_name: str, config):
    """
    Standard build pipeline: Setup -> Clean(optional) -> Configure -> Build
    """
    start_time = time.monotonic()
    
    project_dir = setup_environment()
    build_dir = project_dir / build_dir_name
    source_dir = Path(config['build']['cmake_source_dir'])
    if not source_dir.is_absolute():
        source_dir = (project_dir / source_dir).resolve()
    
    # Check for 'clean' arg in sys.argv
    should_clean = 'clean' in sys.argv
    
    handle_clean(build_dir, config['build']['generator'], should_clean)
    
    try:
        ensure_cmake_configured(
            build_dir=build_dir, 
            build_type=build_type, 
            generator=config['build']['generator'], 
            source_dir=str(source_dir),
            compiler=config['build'].get('compiler', "")
        )
        
        print(f"==> Compiling the project ({build_type} Mode)...")
        build_target(build_dir)
        
        # Summary
        end_time = time.monotonic()
        duration = int(end_time - start_time)
        m, s = divmod(duration, 60)
        
        print("\n================================================================")
        print(f"{build_type} build complete!")
        print(f"Total build time: {m}m {s}s")
        print("================================================================")
        
    except Exception as e:
        print(f"\n!!! An unexpected error occurred: {e}")
        sys.exit(1)

def run_target_only(build_type: str, build_dir_name: str, target: str, config):
    """
    Run a specific target (e.g. format, tidy-fix)
    """
    try:
        project_dir = setup_environment()
        build_dir = project_dir / build_dir_name
        source_dir = Path(config['build']['cmake_source_dir'])
        if not source_dir.is_absolute():
            source_dir = (project_dir / source_dir).resolve()
        
        ensure_cmake_configured(
            build_dir=build_dir, 
            build_type=build_type, 
            generator=config['build']['generator'], 
            source_dir=str(source_dir),
            compiler=config['build'].get('compiler', "")
        )
        
        print(f"==> Running target: {target}...")
        build_target(build_dir, target=target)
        print(f"\n==> Target '{target}' finished successfully.")
        
    except Exception as e:
        print(f"\n!!! An unexpected error occurred: {e}")
        sys.exit(1)
