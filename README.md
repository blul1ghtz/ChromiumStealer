========== V10 CHROMIUMSTEALER ==========

Please do not complain about this not being a whole project
this is only a snippet of a larger project like a infostealer

This is a tool/snippet of a infostealer
that allows you to take encrypted data
from sqlite databases managed by chromium
based browsers, this snippet is not 
limited to google chrome, if you lay it
out with files form other chromium based
browsers it SHOULD decrypt that aswell.
(THIS REPOSITORY IS FOR EDUCATIONAL PURPOSES ONLY,
I TAKE NO RESPONSIBILITY FOR OTHER PEOPLES ACTIONS)


========== REQUIREMENTS ==========
libsodium
sqlite3


========== WHAT THIS PROGRAM DOES ==========

1. The chromium stealer function will get the
master key from chromes local state file in the
function "GetChromiumBasedMasterKey", using regex,
The master key is then stored in a variable
for later decryptions. (you can also use json
but regex is superior)

3. Next it uses sqlite3 to open logins and other
critical files that store personal information,
the information that is encrypted with be
decrypted by "DecryptChromiumBased". (also passing
through masterkey for decryption)

4. "DecryptChrommiumBased" does its magic with the
encrypted string, master key and aes and returns
a decrypted string with the original password.

5. Information is then printed to console.


========== HOW THE DECRYPTION WORKS =========

Below are the specifically for people who care 
about the decryption and how it works exactly

1. Grabs masterkey from chromes local state
  
2. Get information via sqlite3 and attempts
to decrypt it using below method.

3. First 3 bytes of encrypted password is the
version, it is removed from the encrypted
password entirely. (will likely be V10 or V20)

4. The next 12 bytes (excluding v10) is the iv
that you will pass through the AES nonce
parameter.

5. The rest of the encrypted password after version
tag 3 + 12 bytes is the real password to pass through
the ciphertext parameter

6. These were not full instructions cus im not a teacher,
read through the code yourself if you have any additional
questions.

========== NOTES ==========

1. I'm pretty inexperienced with visual
studio soo i included base64.h and base64.cpp
as there are a few base64 libraries, this project
was statically built and idk if people rather to
dynamically include stuff soo that might be in issue.

2. With V20 coming out i would appreciate if anybody could give me any info on how to decrypt V20
(like some special dll injection method or something)
