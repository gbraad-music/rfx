#!/usr/bin/make -f
# Master Makefile for building all individual RFX plugins

PLUGINS = RFX_Distortion RFX_Filter RFX_EQ RFX_Compressor RFX_Delay TB303_Synth

.PHONY: all clean $(PLUGINS) gen-ttl lv2 lv2-ui lv2-dsp vst3 vst3-ui vst3-dsp

all: lv2 vst3 gen-ttl

# Build plugins with UI
lv2: lv2-ui
vst3: vst3-ui

# Build plugins without UI (DSP only)
lv2-dsp: $(addsuffix -lv2-dsp,$(PLUGINS))
vst3-dsp: $(addsuffix -vst3-dsp,$(PLUGINS))

# Build plugins with UI  
lv2-ui: $(addsuffix -lv2,$(PLUGINS))
vst3-ui: $(addsuffix -vst3,$(PLUGINS))

$(PLUGINS):
	@echo "Building $@ (LV2 + VST3)..."
	@$(MAKE) -C $@

%-lv2:
	@echo "Building $(patsubst %-lv2,%,$@) LV2 with UI..."
	@$(MAKE) -C $(patsubst %-lv2,%,$@) lv2_sep

%-lv2-dsp:
	@echo "Building $(patsubst %-lv2-dsp,%,$@) LV2 DSP-only..."
	@$(MAKE) -C $(patsubst %-lv2-dsp,%,$@) lv2_dsp

%-vst3:
	@echo "Building $(patsubst %-vst3,%,$@) VST3 with UI..."
	@$(MAKE) -C $(patsubst %-vst3,%,$@) vst3

%-vst3-dsp:
	@echo "Building $(patsubst %-vst3-dsp,%,$@) VST3 DSP-only..."
	@$(MAKE) -C $(patsubst %-vst3-dsp,%,$@) SKIP_UI=true vst3

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
