# Recovery

Prepare recovery before installation: keep your own known-good stock boot,
matching host tools and a charged device. This repository intentionally does
not bundle OEM firmware or emergency download loaders.

If Android does not start:

1. Stop repeated boot attempts and do not experiment with other partitions.
2. Try the device's normal bootloader/recovery path.
3. Restore only `boot_a` with the known-good image for the exact firmware.
4. If the device cannot reach the bootloader, use only a previously validated
   device-specific emergency procedure and its independently verified tools.

Never erase, format, change the active slot or flash images from another model.
Do not assume that a successful USB connection proves the image is compatible.

