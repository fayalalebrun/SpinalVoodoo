DSLX := $(wildcard dslx/*.x)
ROOT := ../..
RTL_DIR := $(ROOT)/build/generated/xls/rtl
RTL := \
	$(RTL_DIR)/VoodooXlsFramebufferAddress.v \
	$(RTL_DIR)/VoodooXlsWritePack.v \
	$(RTL_DIR)/VoodooXlsDepthCompare.v \
	$(RTL_DIR)/VoodooXlsAlphaBlend.v \
	$(RTL_DIR)/VoodooXlsFogWDepth.v \
	$(RTL_DIR)/VoodooXlsFogTableFactor.v \
	$(RTL_DIR)/VoodooXlsFogBlend.v \
	$(RTL_DIR)/VoodooXlsColorCombine.v \
	$(RTL_DIR)/VoodooXlsLfbDecode.v \
	$(RTL_DIR)/VoodooXlsPixelTests.v \
	$(RTL_DIR)/VoodooXlsDataSwizzle.v \
	$(RTL_DIR)/VoodooXlsTexelDecode.v \
	$(RTL_DIR)/VoodooXlsTextureFilter.v \
	$(RTL_DIR)/VoodooXlsTextureLayout.v \
	$(RTL_DIR)/VoodooXlsTmuRecipMeta.v \
	$(RTL_DIR)/VoodooXlsTmuRecip.v \
	$(RTL_DIR)/VoodooXlsTmuPerspectiveCoords.v \
	$(RTL_DIR)/VoodooXlsTmuGradient.v \
	$(RTL_DIR)/VoodooXlsTmuLodIndex.v \
	$(RTL_DIR)/VoodooXlsTmuBaseLod.v \
	$(RTL_DIR)/VoodooXlsTmuLodSelect.v \
	$(RTL_DIR)/VoodooXlsTmuCoordinates.v \
	$(RTL_DIR)/VoodooXlsTmuAddresses.v \
	$(RTL_DIR)/VoodooXlsTmuPackedDecode.v \
	$(RTL_DIR)/VoodooXlsTextureWriteAddress.v \
	$(RTL_DIR)/VoodooXlsTriangleIterLoad.v \
	$(RTL_DIR)/VoodooXlsTriangleIterStep.v \
	$(RTL_DIR)/VoodooXlsSignedMagnitude16.v \
	$(RTL_DIR)/VoodooXlsTriangleSetup.v \
	$(RTL_DIR)/VoodooXlsTriangleSetupS16.v \
	$(RTL_DIR)/VoodooXlsSpanStep.v

OUTPUTS := $(RTL) $(RTL_DIR)/.stamp

.PHONY: all test clean

all: $(OUTPUTS)

$(OUTPUTS) &: $(DSLX) ../../workflows/build/generate-xls-rtl ../../flake.nix ../../flake.lock
	../../workflows/build/generate-xls-rtl

test:
	@set -e; for source in $(DSLX); do interpreter_main --dslx_path=dslx "$$source"; done

clean:
	rm -rf $(ROOT)/build/generated/xls
