.DEFAULT_GOAL := deploy

build:
	mkdir build

build/fw.zip: build
	mos build --platform esp8266 --local --build-docker-extra --userns=keep-id

.PHONY: compile
compile: build/fw.zip

deploy: build/fw.zip
	source ./.env && mos flash

.PHONY: clean
clean:
	rm -rf build
