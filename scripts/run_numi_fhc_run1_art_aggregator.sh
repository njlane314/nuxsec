#!/usr/bin/env bash
set -euo pipefail

build/bin/nuxsec art "beam_s0:inputs/numi_fhc_run1/filelists/beam_s0.list:overlay:numi"
build/bin/nuxsec art "beam_s1:inputs/numi_fhc_run1/filelists/beam_s1.list:overlay:numi"
build/bin/nuxsec art "beam_s2:inputs/numi_fhc_run1/filelists/beam_s2.list:overlay:numi"
build/bin/nuxsec art "beam_s3:inputs/numi_fhc_run1/filelists/beam_s3.list:overlay:numi"
build/bin/nuxsec art "dirt_s0:inputs/numi_fhc_run1/filelists/dirt_s0.list:dirt:numi"
build/bin/nuxsec art "dirt_s1:inputs/numi_fhc_run1/filelists/dirt_s1.list:dirt:numi"
build/bin/nuxsec art "strangeness_run1_fhc:inputs/numi_fhc_run1/filelists/strangeness_run1_fhc.list:strangeness:numi"
build/bin/nuxsec art "ext_p0:inputs/numi_fhc_run1/filelists/ext_p0.list:ext:numi"
build/bin/nuxsec art "ext_p1:inputs/numi_fhc_run1/filelists/ext_p1.list:ext:numi"
build/bin/nuxsec art "ext_p2:inputs/numi_fhc_run1/filelists/ext_p2.list:ext:numi"
