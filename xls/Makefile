DSLX := $(wildcard dslx/*.x)
RTL := \
	rtl/VoodooXlsFramebufferAddress.v \
	rtl/VoodooXlsWritePack.v \
	rtl/VoodooXlsDepthCompare.v \
	rtl/VoodooXlsAlphaBlend.v \
	rtl/VoodooXlsFogWDepth.v \
	rtl/VoodooXlsFogTableFactor.v \
	rtl/VoodooXlsFogBlend.v \
	rtl/VoodooXlsColorCombine.v \
	rtl/VoodooXlsLfbDecode.v \
	rtl/VoodooXlsPixelTests.v \
	rtl/VoodooXlsDataSwizzle.v \
	rtl/VoodooXlsTexelDecode.v \
	rtl/VoodooXlsTextureFilter.v \
	rtl/VoodooXlsTextureLayout.v \
	rtl/VoodooXlsTmuRecipMeta.v \
	rtl/VoodooXlsTmuRecip.v \
	rtl/VoodooXlsTmuPerspectiveCoords.v \
	rtl/VoodooXlsTmuGradient.v \
	rtl/VoodooXlsTmuLodIndex.v \
	rtl/VoodooXlsTmuBaseLod.v \
	rtl/VoodooXlsTmuLodSelect.v \
	rtl/VoodooXlsTmuCoordinates.v \
	rtl/VoodooXlsTmuAddresses.v \
	rtl/VoodooXlsTmuPackedDecode.v \
	rtl/VoodooXlsTextureWriteAddress.v \
	rtl/VoodooXlsTriangleIterLoad.v \
	rtl/VoodooXlsTriangleIterStep.v \
	rtl/VoodooXlsSignedMagnitude16.v \
	rtl/VoodooXlsTriangleSetup.v \
	rtl/VoodooXlsSpanStep.v

OUTPUTS := $(RTL) rtl/.stamp

.PHONY: all test clean

all: $(OUTPUTS)

$(OUTPUTS) &: $(DSLX) ../scripts/generate-xls-rtl ../flake.nix ../flake.lock
	../scripts/generate-xls-rtl

test:
	@set -e; for source in $(DSLX); do interpreter_main --dslx_path=dslx "$$source"; done

clean:
	rm -rf build rtl
