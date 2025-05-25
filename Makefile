default:
	@+make -C build

clean:
	@rm -rf build/objects/*
	@rm -f sample
	@echo "Clean!"
