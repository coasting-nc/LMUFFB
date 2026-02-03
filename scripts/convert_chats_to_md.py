import os
import json
import glob
from pathlib import Path
from datetime import datetime

def format_iso_timestamp(iso_str):
    """Parses ISO 8601 string and returns a readable date string."""
    if not iso_str or iso_str == 'N/A':
        return iso_str
    try:
        # Parse ISO format (e.g., 2025-11-08T19:04:12.981Z)
        # Note: Python 3.11+ supports fromisoformat properly with Z, but for broader compat:
        dt = datetime.fromisoformat(iso_str.replace('Z', '+00:00'))
        return dt.strftime('%Y-%m-%d %H:%M:%S')
    except ValueError:
        return iso_str

def convert_chats_to_md():
    # Define paths
    home_dir = Path.home()
    gemini_tmp_base = home_dir / ".gemini" / "tmp"
    output_dir = Path("gemini_chats")

    # Create output directory if it doesn't exist
    output_dir.mkdir(exist_ok=True)
    print(f"Output directory: {output_dir.resolve()}")

    # Find all session json files
    # Pattern: ~/.gemini/tmp/*/chats/session-*.json
    search_pattern = str(gemini_tmp_base / "*" / "chats" / "session-*.json")
    json_files = glob.glob(search_pattern)

    if not json_files:
        print("No chat sessions found in .gemini/tmp")
        return

    print(f"Found {len(json_files)} chat sessions.")

    for json_file in json_files:
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # Extract basic info
            session_id = data.get('sessionId', 'unknown_session')
            
            # JSON-internal dates
            json_start_time = format_iso_timestamp(data.get('startTime', 'N/A'))
            json_last_updated = format_iso_timestamp(data.get('lastUpdated', 'N/A'))

            # File system dates
            fs_creation_time = os.path.getctime(json_file)
            fs_mod_time = os.path.getmtime(json_file)
            
            fs_creation_str = datetime.fromtimestamp(fs_creation_time).strftime('%Y-%m-%d %H:%M:%S')
            fs_mod_str = datetime.fromtimestamp(fs_mod_time).strftime('%Y-%m-%d %H:%M:%S')

            # Extract first message timestamp if available
            messages = data.get('messages', [])
            first_msg_time = 'N/A'
            if messages:
                first_msg_time = format_iso_timestamp(messages[0].get('timestamp', 'N/A'))
            
            # Create a filename
            base_name = Path(json_file).stem
            md_filename = f"{base_name}.md"
            output_path = output_dir / md_filename
            
            # Build Markdown content
            md_content = []
            md_content.append(f"# Chat Session: {base_name}")
            
            # Detailed Timestamp Info Block
            md_content.append("### Timestamp Information")
            md_content.append(f"- **JSON Start Time:** {json_start_time}")
            md_content.append(f"- **First Message Time:** {first_msg_time}")
            md_content.append(f"- **JSON Last Updated:** {json_last_updated}")
            md_content.append(f"- **File Creation Time (OS):** {fs_creation_str}")
            md_content.append(f"- **File Modification Time (OS):** {fs_mod_str}")
            md_content.append("\n---\n")
            
            for msg in messages:
                role = msg.get('type', 'unknown').capitalize()
                content = msg.get('content', '')
                timestamp = format_iso_timestamp(msg.get('timestamp', ''))
                
                # Header for each message
                md_content.append(f"## {role} ({timestamp})")
                
                # Add content (thoughts are excluded)
                if content:
                    md_content.append(content)
                
                md_content.append("\n---\n")

            # Write to file
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write("\n".join(md_content))
            
            print(f"Converted: {base_name} -> {output_path}")

        except Exception as e:
            print(f"Error converting {json_file}: {e}")

if __name__ == "__main__":
    convert_chats_to_md()
