import os

def print_directory_tree(directory, prefix=""):
    for item in os.listdir(directory):
        full_path = os.path.join(directory, item)
        if os.path.isdir(full_path):
            print(f"{prefix}©À©¤©¤ {item}/")
            print_directory_tree(full_path, prefix + "©¦   ")
        else:
            print(f"{prefix}©¸©¤©¤ {item}")

# Replace 'your_project_directory' with the actual path to your project directory
project_directory = ''
print_directory_tree(project_directory)