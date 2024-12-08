# FEX tagged version (release) process
A FEX tagged version happens near the start of each month.

The tagged versioning is `FEX-<YYMM>` with the month being the current month.

If a tagged version was being done on `Sun, 02 Jan 2022` then the FEX version would be FEX-2201

There are multiple locations that need to be updated during a release
* Github tagged release
* Github releases page
* fex-emu.com blog post
* https://launchpad.net/~fex-emu/+archive/ubuntu/fex Ubuntu PPA
* @FEX_Emu twitter account

* Optional: Update the rootfs images

## Github Steps
* Check out the commit that will be the branch

  $ git checkout upstream/main

* Make local main branch be the selected commit

  $ git branch -D main
  $ git checkout -b main

* Run the release script

  $ Scripts/generate_release.sh

* Push the branches upstream
  * This requires administrative push rights
  * Both the tag and the main branch needs to be committed

  $ git push upstream $CURRENT
  $ git push upstream main

## Launchpad PPA steps
Follow the steps in: https://github.com/FEX-Emu/FEX-ppa/blob/main/README.md
* Requires PPA GPG key signing access
* Wait the 20-30 minutes for Ubuntu PPA to build and publish the binaries

## Github releases page Steps
* Requires administrative rights
* Go to https://github.com/FEX-Emu/FEX/releases
* Click Draft a new release
* Copy and paste the tagged changelog in to the draft release markdown
  * This was generated from the generate_release.sh script
* Clean the markdown to a desired level of combining and ordering
  * Fairly trivial cleanups, it's more just a developer focused changelog
* Click publish release

## fex-emu.com blog post steps
* clone https://github.com/FEX-Emu/fex-emu.com
* Copy the previous post from the _posts/ folder to a new markdown file
  * Ensure correct date format in filename
* Copy github release pages markdown in to this
* Easy to forget areas:
  * Title text section
  * See Release notes top section, links to github release tag
  * See detailed changelog at the bottom, linking to github raw revision comparison
* Short blurb in the top paragraph if desired
* push new md file to the repo. Either in direct push or PR
* Jekyll will automatically regenerate the website with a github action
* Verify that the post shows up on the site at fex-emu.com

## @FEX_Emu twitter account steps
* Requires @FEX_Emu twitter account access
* Create a tweet with some small blurb/sizzle text about some relevant changes in this tagged version
* Link to the fex-emu.com blog post about the change

## RootFS image updating
* This doesn't typically need to be done on a monthly basis
* This lives in https://github.com/FEX-Emu/RootFS

* Follow the Build_Data file's information for how to generate an image using `build_image.py`
  * This gives a squashfs image for the rootfs
* Use FEXRootFSFetcher <image.sqsh> to generate the xxhash for the image
* Update `https://rootfs.fex-emu.com/file/fex-rootfs/RootFS_links.json` with the new rootfs image and hash
  * This currently lives in a private FEX-Emu backblaze bucket with cloudflare servicing it.
  * Never publicly give the direct backblaze link to the file. Will cause BW costs to skyrocket
  * Always pass through cloudflare

* Upload new image to Backblaze using the b2 upload tool
  * b2 upload-file <bucketname> <image.sqsh> <Image folder name>/<image.sqsh>

* Upload the new RootFS_links.json
  * Lives in the root of the bucket
  * b2 upload-file <bucketname> RootFS_links.json RootFS_links.json

* Once uploaded it should propagate immediately
* Might be worth thinking about the coherency problem of updating the hash versus image independently if overwriting an image
  * Need to be careful about it to not break anyone in the process of downloading an image
