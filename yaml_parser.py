import yaml
import json
import os
import sys

def parse_yaml_to_json(yaml_file_path):
    """
    Parse a YAML file and return its contents as JSON.
    This allows easy passing of data between Python and C++.
    
    Args:
        yaml_file_path (str): Path to the YAML file
        
    Returns:
        str: JSON string representation of the YAML content
    """
    try:
        # Try multiple paths to find the YAML file
        paths_to_try = [
            yaml_file_path,                                  # Direct path
            os.path.join("..", yaml_file_path),              # One level up
            os.path.join(os.getcwd(), yaml_file_path),       # Current working directory
            os.path.join(os.path.dirname(os.getcwd()), yaml_file_path)  # Parent directory
        ]
        
        for path in paths_to_try:
            if os.path.exists(path):
                with open(path, 'r') as file:
                    yaml_content = yaml.safe_load(file)
                return json.dumps(yaml_content)
        
        return json.dumps({"error": f"File not found: {yaml_file_path}"})
            
    except Exception as e:
        return json.dumps({"error": f"Error parsing YAML: {str(e)}"})

def save_json_to_file(json_data, output_file_path):
    """
    Save JSON data to a file.
    
    Args:
        json_data (str): JSON string to save
        output_file_path (str): Path to save the JSON file
        
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        with open(output_file_path, 'w') as file:
            file.write(json_data)
        return True
    except Exception as e:
        print(f"Error saving JSON: {str(e)}", file=sys.stderr)
        return False

if __name__ == "__main__":
    # When called as a script, parse args and output JSON
    if len(sys.argv) < 2:
        print("Usage: python yaml_parser.py <yaml_file_path> [output_json_path]", file=sys.stderr)
        sys.exit(1)
        
    yaml_file_path = sys.argv[1]
    json_data = parse_yaml_to_json(yaml_file_path)
    
    if len(sys.argv) >= 3:
        # Save to file if output path provided
        output_file_path = sys.argv[2]
        if save_json_to_file(json_data, output_file_path):
            print(f"YAML parsed and saved to {output_file_path}")
        else:
            print(f"Failed to save JSON to {output_file_path}", file=sys.stderr)
            sys.exit(1)
    else:
        # Otherwise print to stdout
        print(json_data)