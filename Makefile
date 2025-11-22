SRC := src
BIN := bin
DIST := dist

# All .z files in src
Z_FILES := $(shell find $(SRC) -type f -name "*.z")
Z_PAIRS := $(foreach f,$(Z_FILES), \
	$(f) $(BIN)/$(patsubst $(SRC)/%,%,$(patsubst %.z,%.py,$(f))) \
)

# Zink build command
ZINK := python -m zlang --lang py --pretty --verbose

.PHONY: build clean

all: build


clean:
	@rm -rf "$(BIN)"

setup: clean
	@find $(SRC) -type d -exec sh -c '\
		rel="$${1#$(SRC)/}"; \
		[ "$$rel" = "$(SRC)" ] && rel=""; \
		mkdir -p "$(BIN)/$$rel"; \
	' _ {} \;


build: setup $(BIN)
	@find "$(SRC)" -type f ! -name "*.z" -exec sh -c '\
		src="$$1"; \
		rel="$${src#$(SRC)/}"; \
		cp "$$src" "$(BIN)/$$rel"; \
	' _ {} \;
ifneq ($(Z_FILES),)
	@$(ZINK) $(Z_PAIRS)
endif

$(BIN):
	@mkdir -p "$(BIN)"

install:
	python -m pip install zlang

flash:
	mpremote cp -r bin/* :/

run: flash
	mpremote reset
	sleep 2
	mpremote

release:
	mkdir -p $(DIST)
	cd $(BIN);\
	tar -cvf ../$(DIST)/deimos.tar \
	--exclude user \
	--exclude apps/axios.py \
	--exclude apps/changed.py \
	--exclude conf/axios.txt \
	--exclude conf/wifi.txt \
	$(shell ls $(BIN))
	gh release create "" \
	$(DIST)/deimos.tar