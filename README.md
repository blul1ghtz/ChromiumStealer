========== V10 CHROMIUMSTEALER ==========

This is a tool/snippet of a infostealer
that allows you to take encrypted data
from sqlite databasesmanaged by chromium
based browsers, this snippet is not 
limited to google chrome, if you lay it
out with files form other chromium based
browsers it SHOULD decrypt that aswell.


========== REQUIREMENTS ==========
libsodium
sqlite3


========== WHAT THIS PROGRAM DOES ==========

1. The chromium stealer function will get the
master key from chromes local state file 
using regex, The master key is then stored in
a variable for later decryptions.
(you can also use json butregex is superior)

2. Next it uses sqlite3 to open logins and other
critical files that store personal information,
the information that is encrypted with be
decrypted by DecryptChromiumBased. (also passing
through masterkey for decryption)

3. DecryptChrommiumBased does its  magic with the
encrypted string, master key and aes and returns
a decrypted string with the original password.

4. Information is then printed to console.


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
read through the code yourself  if you have any additional
questions.

========== NOTES ==========

1. I'm pretty inexperienced with visual
studio soo i included base64.h and abse64.cpp
as there are a few base64 libraries, this project
was statically built and idk if people rather to
dynamically stuff soo imma include all of the
other files i include in the project in other.

3. With V20 coming out i would appreciate if anybody could give me any info on how to decrypt V20
(like some special dll injection method or something)
