%ifdef CONFIG
{
  "RegData": {
    "RDX": "0"
  },
  "Env": { "FEX_JITFEATURES" : "enablex87fip", "FEX_X87REDUCEDPRECISION" : "0" },
  "Mode": "32BIT"
}
%endif

%include "x87_FIP.mac"
