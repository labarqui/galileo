To build the kernel follow [AlexT's instrucions](https://github.com/alext-mkrs/meta-alext-galileo):

1. Download this: [http://downloadmirror.intel.com/23197/eng/Board_Support_Package_Sources_for_Intel_Quark_v1.0.1.7z](http://downloadmirror.intel.com/23197/eng/Board_Support_Package_Sources_for_Intel_Quark_v1.0.1.7z)

2. Extract:

    ```
    tar xzvf meta-clanton_v1.0.1.tar.gz
    cd meta-clanton_v1.0.1
    git clone https://github.com/alext-mkrs/meta-alext-galileo
    ./setup.sh -e "meta-alext-galileo"
    ```
    
3. Remove OpenCV bindings:

    ```
    # Comment OpenCV entries and python bindings
    vim ./meta-clanton-distro/recipes-core/images/image-full.bb
    # Remove files:
    rm ./meta-clanton-distro/recipes-support/opencv/opencv_2.4.3.bbappend
    rm ./meta-oe/meta-oe/recipes-support/opencv/opencv-samples_2.4.3.bb
    ```

4. Setup environment:

    ```
    source poky/oe-init-build-env yocto_build/
    bitbake linux-yocto-clanton
    bitbake image-full-galileo
    ```

Once the kernel is built, rebuild modules with:

```
bitbake linux-yocto-clanton
bitbake image-full-galileo
```

To compress in a single tar archive:

```
cd tmp/deploy/images/
tar -hcvf ~/image.tar bzImage boot/ core-image-minimal-initramfs-clanton.cpio.gz grub.efi image-full-galileo-clanton.ext3
```
