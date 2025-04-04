# Currently this is configured to build both projects (the eco process is basically dead)
# If configuring with an IDE, set the Makefile directory to src/aroma or src/eco_process respectively.

all: $(BUILD)

$(BUILD):
	make -C src/aroma
	cp src/aroma/Ristretto.wps .
	cp src/aroma/Ristretto.elf .

	# Most likely an alternative to this will be found for power on
	# but until 100% sure a solution is found, disable building eco process replacement
	# make -C src/eco_process
	# cp src/eco_process/eco_process.elf .
	# cp src/eco_process/eco_process.rpx .

clean:
	make -C src/aroma clean
	# make -C src/eco_process clean
	# rm -fr Ristretto.wps Ristretto.elf eco_process.elf eco_process.rpx
	rm -fr Ristretto.wps Ristretto.elf
