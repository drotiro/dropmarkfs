# Dropmarkfs

FUSE filesystem for the Dropmark cloud service.

You need an API key to run this software.


## Compiling

* Make sure you have all the required dependencies:
 * libapp  - https://github.com/drotiro/libapp
 * rest-friend - https://github.com/drotiro/rest-friend
* Run `make` and optionally `sudo make install` to get the headers and the library
installed under `PREFIX`. 

If you prefer a static build or don't want to install the dependencies system-wide,
you can run `make static` to:
 * download (`git clone`) libapp and rest-friend inside the source tree
 * compile them
 * link against those local libs


## Running 

Run `dmfs` providing at least the key file and the mount point, like the following example:

`dmfs -l youremail@domain.com -k file_with_api_key /mountpoint`


## Copyright and license

This software is written by me Domenico Rotiroti.

Dropmarkfs is licensed under the GPLv3 (see gpl-3.0.txt) and it comes without any warranty.
