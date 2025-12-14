#!/usr/bin/make -f
# Master Makefile for building all individual RFX plugins

PLUGINS = RFX_Distortion RFX_Filter RFX_EQ RFX_Compressor RFX_Delay

.PHONY: all clean $(PLUGINS) gen-ttl

all: $(PLUGINS) gen-ttl

$(PLUGINS):
	@echo "Building $@..."
	@$(MAKE) -C $@ lv2_sep

clean:
	@for plugin in $(PLUGINS); do \
		echo "Cleaning $$plugin..."; \
		$(MAKE) -C $$plugin clean; \
	done

install:
	@for plugin in $(PLUGINS); do \
		echo "Installing $$plugin..."; \
		$(MAKE) -C $$plugin install; \
	done

# Generate TTL files for all plugins
gen-ttl:
	@echo "Generating LV2 TTL metadata files..."
	@for plugin in $(PLUGINS); do \
		if [ -f ../bin/$$plugin.lv2/$${plugin}_dsp.so ]; then \
			echo "  - $$plugin"; \
			(cd ../bin/$$plugin.lv2 && ../../dpf/utils/lv2_ttl_generator ./$${plugin}_dsp.so > /dev/null 2>&1); \
		fi; \
	done
	@echo "Done!"
