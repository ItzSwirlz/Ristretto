all:
	make -C src/aroma
	cp src/aroma/Ristretto.wps .
	cp src/aroma/Ristretto.elf .

	make -C src/eco_process
	cp src/eco_process/eco_process.elf .
	cp src/eco_process/eco_process.rpx .

clean:
	make -C src/aroma clean
	make -C src/eco_process clean
	rm -fr Ristretto.wps Ristretto.elf eco_process.elf eco_process.rpx
