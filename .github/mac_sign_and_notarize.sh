#!/bin/sh
# When downloading software from the internet, macOS sets a "quarantine"
# attribute on the downloaded file that makes macOS's Gatekeeper check that the
# software is signed by a known developer. Starting with macOS Catalina,
# Gatekeeper also checks that the software is notarized, and also requires that
# command-line tools are signed (previously, command-line tools were not
# quaratined like ".app"s were).
#
# Signing means that someone with an Apple Developer ID ran 'codesign' or
# 'productsign' on the software. This tells users that the software they have
# obtained has been acknowledged/signed by a developer that Apple trusts.
# Notarizing means that a developer uploaded their software to Apple's servers
# and Apple ran some checks to ensure the software is secure.
#
# A user's computer does not need internet access to check that software is
# signed. However, checking that software is notarized requires an internet
# connection, UNLESS the notarization has been stapled to the software.
#
# This script takes a Moco ZIP, unzips it to a temporary directory, signs all
# executables and shared libraries, re-zips, then submits the ZIP to Apple to
# be notarized. You will get an email indicating if notarization succeeded;
# though this might take a few hours.
#
# The resulting signed ZIP is saved to a folder <zip-name>-signed. This ZIP is
# what should be provided to users.
#
# This script takes three arguments: the Developer ID identity (-s), the
# Apple ID associated with the Developer ID (-i) and the ZIP to
# sign and notarize.
#
# Example:
# ./mac_sign_and_notarize.sh -s ABC0123456 -i <Apple-ID> -f opensim-moco.zip
#
# Run `security find-identity -p basic -v` to get the Developer ID identity; 
# it's in parentheses at the end of the line. You must have installed a
# 'certificate'; ask chrisdembia or aymanhab for a certificate file. The Apple
# ID is your Apple ID that is signed into Xcode and associated with the
# Developer ID. See Xcode > Preferences > Accounts.
#
# This script assumes that you've stored your Apple ID password in the macOS
# Keychain. Open "KeyChain Access.app" and add an entry to the login keychain
#   Keychain Item Name: Developer-altool
#   Account Name: <Apple-ID>
#   Password: <Apple-ID-password>
# (The name Developer-altool is what we assume the keychain item is named in
# this script; this name is just a convention).
#
# Ideally, we would also staple the notarization to the ZIP, but the stapler
# command currently only works on ".app"s, ".pkg"s, and ".dmg"s, and not on
# ".zip"s or plain executables. So, for now, Moco users will need access to
# the internet for their Gatekeeper to determine if the software is notarized.
#
# While Apple's documentation says that macOS requires software to be signed
# AND notarized, testing indicates that it is sufficent for software to be only
# signed (not notarized) for Gatekeeper to allow the user to run the software.
# This may change over time.
# 
# https://developer.apple.com/documentation/xcode/notarizing_macos_software_before_distribution
# https://scriptingosx.com/2019/09/notarize-a-command-line-tool/


while getopts s:i:f: option
    do
    case "${option}"
    in
    s) IDENTITY=${OPTARG};;
    i) APPLE_ID=${OPTARG};;
    f) MOCO_ZIP=${OPTARG};;
    esac
done


# Signing.
# ========
function sign_code() {
    codesign \
        --sign $IDENTITY \
        --options runtime \
        --timestamp \
        --deep \
        --force "$@"
}

function cleanup() {
    rm -rf "$MOCO_TEMP_DIR"
    echo "Deleted temporary directory $MOCO_TEMP_DIR."
}

# https://stackoverflow.com/questions/4632028/how-to-create-a-temporary-directory
MOCO_TEMP_DIR=`mktemp -d -t 'opensim-moco-unzipping'`

if [[ ! "$MOCO_TEMP_DIR" || ! -d "$MOCO_TEMP_DIR" ]]; then
    echo "Could not create temporary directory."
    exit 1
fi
echo "Created temporary directory $MOCO_TEMP_DIR."

# Unzip quietly.
unzip -q -d "$MOCO_TEMP_DIR" "$MOCO_ZIP"

# Sign executables and shared libraries.
for dir in `find "$MOCO_TEMP_DIR" -mindepth 1 -maxdepth 1 -type d`; do
    for i in `find "$dir" -name "*.dylib" -type f`; do
        sign_code "$i"
    done
    # For the shared libraries in the Python bindings:
    for i in `find "$dir" -name "*.so" -type f`; do
        sign_code "$i"
    done
    sign_code "$dir/bin/opensim-cmd"
    sign_code "$dir/bin/opensim-moco"
    sign_code "$dir/sdk/Simbody/libexec/simbody/simbody-visualizer.app"
done

# ZIP the signed files.
mkdir -p ${MOCO_ZIP}-signed
ORIG_DIR=`pwd`
cd $MOCO_TEMP_DIR
zip --quiet --symlinks --recurse-paths \
    "${ORIG_DIR}/${MOCO_ZIP}-signed/${MOCO_ZIP}" .
cd - 


# Notarizing.
# ===========
xcrun altool --notarize-app --primary-bundle-id "org.opensim.moco" \
    --username "$APPLE_ID" \
    --password "@keychain:Developer-altool" \
    --asc-provider "$IDENTITY" \
    --file "${ORIG_DIR}/${MOCO_ZIP}-signed/${MOCO_ZIP}"

# If notarization fails, you'll get an email from Apple with a RequestUUID.
# Run the following command:
#   xcrun altool --notarization-info "<RequestUUID>" --username "$APPLE_ID" \
#       --password "@keychain:Developer-altool"
# This will output a URL for a webpage with details about why the notarization
# failed. Be sure to view this webpage promptly; the page is deleted after a
# while.

# TODO: stapling.
# For example:
# xcrun stapler staple <app-or-pkg>
# The scriptingosx.com link above provides a handy way to wait until the
# notarization succeeds (by polling Apple's servers with `xcrun altool
# --notarization-info`) to run the stapling.

trap cleanup EXIT
