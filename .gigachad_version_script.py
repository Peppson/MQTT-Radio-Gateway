
#
# How does python even work!?
#

Import("env")
FILE_NAME = "src/main.cpp" 


# Increment by 1 and convert back to string (inefficient)
def increment_version_and_convert_str(cur_version):
    A, B, C = map(int, cur_version.split("."))
    int_version = (A * 100 + B * 10 + C) + 1
    if (int_version > 999):
        int_version = 0
    return f"{int_version // 100}.{(int_version // 10) % 10}.{int_version % 10}"


# Increment Build or update Uploaded version?
def build_or_upload(bool_build): 
    with open(FILE_NAME, "r") as f: 
        lines = f.readlines()

    # Find matching string and replace
    for i in range(len(lines)):
        if ("#   - Build version: ") in lines[i]:
            cur_version = lines[i].split(": ")[1][:5]

            # Build
            if bool_build:
                new_version = increment_version_and_convert_str(cur_version)
                lines[i] = lines[i].replace(cur_version, new_version)
            # Upload
            else:
                if ("#   - Uploaded version: ") in lines[i-1]:
                    prev_line = lines[i-1].split(": ")[1][:5]
                    lines[i-1] = lines[i-1].replace(prev_line, cur_version)
            with open(FILE_NAME, "w") as f: 
                f.writelines(lines)
            break

def post_build(source, target, env):
    build_or_upload(1)

def post_upload(source, target, env):
    build_or_upload(0)

env.AddPostAction("buildprog", post_build)
env.AddPostAction("upload", post_upload)


