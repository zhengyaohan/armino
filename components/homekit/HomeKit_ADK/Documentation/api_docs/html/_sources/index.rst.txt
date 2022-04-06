HomeKit ADK
-----------
The HomeKit Accessory Development Kit (ADK) abstracts the details of the HomeKit Accessory Protocol (HAP) so that a
chipset vendor can ultimately focus on their hardware platform and an accessory manufacturer on the application layer.

The HomeKit ADK implements key components of the HomeKit Accessory Protocol (HAP), which embodies the core principles
Apple brings to smart home technology: **security**, **privacy**, and **reliability**.

First steps
-----------
If you are new to ADK then ADK architecture is a good starting point to understand different components that make up
ADK.

- :doc:`ADK Architecture </adk_architecture>`
- :doc:`ADK Directory Structure </adk_directory_structure>`
- :doc:`Docker with ADK </docker>`

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Introduction

   adk_architecture.md
   adk_directory_structure.md
   docker.md

Step-by-step Guides
-------------------
These guides will walk you through the steps required to setup your development environment and compile ADK
source.

- :doc:`/getting_started`
- :doc:`/raspi_setup`
- :doc:`/bct`
- :doc:`/wac_on_raspberrypi`
- :doc:`/camera_on_darwin`

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Step-by-step Guides

   getting_started.md
   raspi_setup.md
   bct.md
   wac_on_raspberrypi.md
   camera_on_darwin.md

Troubleshooting ADK
"""""""""""""""""""""
- :doc:`/how_to`
- :doc:`/interact_with_applications`
- :doc:`/troubleshooting`
- :doc:`/writing_unit_tests`
- :doc:`/debug_adk`

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Troubleshooting ADK

   how_to.md
   interact_with_applications.md
   troubleshooting.md
   writing_unit_tests.md
   debug_adk.md

HomeKit ADK Development
-----------------------
- :doc:`/accessory_development`
- :doc:`/accessory_pairing`
- :doc:`/accessory_authentication`
- :doc:`/provisioning_tools`
- :doc:`/ADK_Memory_Usage_and_Requirements`
- :doc:`/crypto`
- :doc:`/make_options`
- :doc:`/coding_convention`
- :doc:`/migration_guide`

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: ADK Development

   accessory_development.md
   accessory_pairing.md
   accessory_authentication.md
   provisioning_tools.md
   ADK_Memory_Usage_and_Requirements.md
   crypto.md
   make_options.md
   coding_convention.md
   migration_guide.md

HomeKit Services
----------------
Accessories are composed of services and characteristics. Services group functionality in order to provide context.
Profiles define a collection of services (and characteristics) and their behaviors that can be supported by an accessory
to provide a consistent user‑experience for a feature. Learn more about implementation of some of these HomeKit Services
and Profiles below.

- :doc:`/accessory_diagnostics`
- :doc:`/accessory_metrics`
- :doc:`/adaptive_light`
- :doc:`/appletv_remote`
- :doc:`/firmware_update`
- :doc:`/homekit_bridge`
- :doc:`/ip_camera`
- :doc:`/lock_profile`
- :doc:`/stateless_programmable_switch`
- :doc:`/thread`
- :doc:`/wifi_reconfiguration_feature`

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: HomeKit Services

   accessory_diagnostics.md
   accessory_metrics.md
   adaptive_light.md
   appletv_remote.md
   firmware_update.md
   homekit_bridge.md
   ip_camera.md
   lock_profile.md
   stateless_programmable_switch.md
   thread
   wifi_reconfiguration_feature.md

Additional Documentation
------------------------
The following documents are available on the MFi portal for additional details and information.

- HomeKit Accessory Protocol Specification

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: PAL APIs

   _api_docs/dir_PAL
