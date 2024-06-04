To launch the master script and the accompanying child scripts, follow these steps:

1. **Compile the scripts**:
   You need to compile the `script1.c`, `script2.c`, and `master.c` files using `gcc`. Ensure you have OpenCV installed and properly configured on your system.

   ```sh
   gcc -o script1 script1.c `pkg-config --cflags --libs opencv4`
   gcc -o script2 script2.c `pkg-config --cflags --libs opencv4`
   gcc -o master master.c
   ```

2. **Set executable permissions** (if needed):
   Ensure the compiled binaries are executable. You can set the permissions using `chmod`.

   ```sh
   chmod +x script1
   chmod +x script2
   chmod +x master
   ```

3. **Run the master script**:
   Simply execute the master script. It will handle launching and managing the child scripts as per the logic defined.

   ```sh
   ./master
   ```

Make sure you have the necessary OpenCV libraries and include files available on your system. If OpenCV is not installed, you can install it using package managers like `apt` for Debian-based systems or `brew` for macOS, or follow the build instructions from the OpenCV website.

**Example Installation on Ubuntu**:

```sh
sudo apt update
sudo apt install -y build-essential cmake pkg-config
sudo apt install -y libopencv-dev
```