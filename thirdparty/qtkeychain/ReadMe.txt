QtKeychain
==========

QtKeychain is a Qt API to store passwords and other secret data securely. How the data is stored depends on the platform:

 * **Mac OS X:** Passwords are stored in the OS X Keychain.

 * **Linux/Unix:** If running, KWallet (via D-Bus) is used.
Support for the GNOME Keyring via freedesktop.org's
[Secret Storage D-Bus specification](http://freedesktop.org/wiki/Specifications/secret-storage-spec "Secret Storage specification") is planned but not yet implemented.

 * **Windows:** Windows does not provide a service for secure storage. QtKeychain uses the Windows API function [CryptProtectData](http://msdn.microsoft.com/en-us/library/windows/desktop/aa380261%28v=vs.85%29.aspx "CryptProtectData function") to encrypt the password with the user's logon credentials. The encrypted data is then persisted via QSettings.

In unsupported environments QtKeychain will report an error. It will never store any data unencrypted.

**License:** QtKeychain is available under the [Modified BSD License](http://www.gnu.org/licenses/license-list.html#ModifiedBSD). See the file COPYING for details.
