###############################################################################
# Author:      Edmund Dea (deae@oregonstate.edu)
# Student ID:  933280343
# Date:        8/10/2019
# Title:       README
###############################################################################

Build Instructions:

1) Copy the following files into the same directory:

	otp_dec.c
	otp_dec_d.c
	otp_enc.c
	otp_enc_d.c
	keygen.c
	plaintext1
	plaintext2
	plaintext3
	plaintext4
	plaintext5
	makefile
	p4gradingscript
	compileall

2) Run the command "./compileall" or "make". This will generate 5 binaries called:

	otp_dec
	otp_dec_d
	otp_enc
	otp_enc_d
	keygen

3) Run Program 3 test script:

	$ ./p4gradingscript PORT1 PORT2 > mytestresults 2>&1
