/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// THIS FILE IS GENERATED BY ZAP

// Prevent multiple inclusion
#pragma once

#include <lib/core/CHIPConfig.h>

// Default values for the attributes longer than a pointer,
// in a form of a binary blob
// Separate block is generated for big-endian and little-endian cases.
#if BIGENDIAN_CPU
#define GENERATED_DEFAULTS                                                                                                         \
    {                                                                                                                              \
                                                                                                                                   \
        /* Endpoint: 0, Cluster: General Commissioning (server), big-endian */                                                     \
                                                                                                                                   \
        /* 0 - Breadcrumb, */                                                                                                      \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                                            \
    }

#else // !BIGENDIAN_CPU
#define GENERATED_DEFAULTS                                                                                                         \
    {                                                                                                                              \
                                                                                                                                   \
        /* Endpoint: 0, Cluster: General Commissioning (server), little-endian */                                                  \
                                                                                                                                   \
        /* 0 - Breadcrumb, */                                                                                                      \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                                            \
    }

#endif // BIGENDIAN_CPU

#define GENERATED_DEFAULTS_COUNT (1)

#define ZAP_TYPE(type) ZCL_##type##_ATTRIBUTE_TYPE
#define ZAP_LONG_DEFAULTS_INDEX(index)                                                                                             \
    {                                                                                                                              \
        &generatedDefaults[index]                                                                                                  \
    }
#define ZAP_MIN_MAX_DEFAULTS_INDEX(index)                                                                                          \
    {                                                                                                                              \
        &minMaxDefaults[index]                                                                                                     \
    }
#define ZAP_EMPTY_DEFAULT()                                                                                                        \
    {                                                                                                                              \
        (uint16_t) 0                                                                                                               \
    }
#define ZAP_SIMPLE_DEFAULT(x)                                                                                                      \
    {                                                                                                                              \
        (uint16_t) x                                                                                                               \
    }

// This is an array of EmberAfAttributeMinMaxValue structures.
#define GENERATED_MIN_MAX_DEFAULT_COUNT 0
#define GENERATED_MIN_MAX_DEFAULTS                                                                                                 \
    {                                                                                                                              \
    }

#define ZAP_ATTRIBUTE_MASK(mask) ATTRIBUTE_MASK_##mask
// This is an array of EmberAfAttributeMetadata structures.
#define GENERATED_ATTRIBUTE_COUNT 13
#define GENERATED_ATTRIBUTES                                                                                                       \
    {                                                                                                                              \
                                                                                                                                   \
        /* Endpoint: 0, Cluster: Access Control (server) */                                                                        \
        { 0x00000000, ZAP_TYPE(ARRAY), 0, ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE),                     \
          ZAP_EMPTY_DEFAULT() }, /* ACL */                                                                                         \
            { 0x00000001, ZAP_TYPE(ARRAY), 0, ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE) | ZAP_ATTRIBUTE_MASK(WRITABLE),                 \
              ZAP_EMPTY_DEFAULT() },                                                                       /* Extension */         \
            { 0x0000FFFB, ZAP_TYPE(ARRAY), 0, ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE), ZAP_EMPTY_DEFAULT() }, /* AttributeList */     \
            { 0x0000FFFD, ZAP_TYPE(INT16U), 2, 0, ZAP_SIMPLE_DEFAULT(1) },                                 /* ClusterRevision */   \
                                                                                                                                   \
            /* Endpoint: 0, Cluster: General Commissioning (server) */                                                             \
            { 0x00000000, ZAP_TYPE(INT64U), 8, ZAP_ATTRIBUTE_MASK(WRITABLE), ZAP_LONG_DEFAULTS_INDEX(0) }, /* Breadcrumb */        \
            { 0x00000001, ZAP_TYPE(STRUCT), 0, ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE),                                               \
              ZAP_EMPTY_DEFAULT() },                                            /* BasicCommissioningInfo */                       \
            { 0x0000FFFD, ZAP_TYPE(INT16U), 2, 0, ZAP_SIMPLE_DEFAULT(0x0001) }, /* ClusterRevision */                              \
                                                                                                                                   \
            /* Endpoint: 0, Cluster: Network Commissioning (server) */                                                             \
            { 0x0000FFFD, ZAP_TYPE(INT16U), 2, 0, ZAP_SIMPLE_DEFAULT(0x0001) }, /* ClusterRevision */                              \
                                                                                                                                   \
            /* Endpoint: 0, Cluster: Operational Credentials (server) */                                                           \
            { 0x00000001, ZAP_TYPE(ARRAY), 0, ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE), ZAP_EMPTY_DEFAULT() }, /* fabrics list */      \
            { 0x00000002, ZAP_TYPE(INT8U), 1, 0, ZAP_EMPTY_DEFAULT() },                                    /* SupportedFabrics */  \
            { 0x00000003, ZAP_TYPE(INT8U), 1, 0, ZAP_EMPTY_DEFAULT() }, /* CommissionedFabrics */                                  \
            { 0x00000004, ZAP_TYPE(ARRAY), 0, ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE),                                                \
              ZAP_EMPTY_DEFAULT() },                                            /* TrustedRootCertificates */                      \
            { 0x0000FFFD, ZAP_TYPE(INT16U), 2, 0, ZAP_SIMPLE_DEFAULT(0x0001) }, /* ClusterRevision */                              \
    }

// This is an array of EmberAfCluster structures.
#define ZAP_ATTRIBUTE_INDEX(index) (&generatedAttributes[index])

#define ZAP_GENERATED_COMMANDS_INDEX(index) ((chip::CommandId *) (&generatedCommands[index]))

// Cluster function static arrays
#define GENERATED_FUNCTION_ARRAYS

// clang-format off
#define GENERATED_COMMANDS { \
  /* Endpoint: 0, Cluster: General Commissioning (server) */\
  /*   client_generated */ \
  0x00000000 /* ArmFailSafe */, \
  0x00000002 /* SetRegulatoryConfig */, \
  0x00000004 /* CommissioningComplete */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   server_generated */ \
  0x00000001 /* ArmFailSafeResponse */, \
  0x00000005 /* CommissioningCompleteResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Network Commissioning (server) */\
  /*   client_generated */ \
  0x00000000 /* ScanNetworks */, \
  0x00000002 /* AddOrUpdateWiFiNetwork */, \
  0x00000003 /* AddOrUpdateThreadNetwork */, \
  0x00000004 /* RemoveNetwork */, \
  0x00000006 /* ConnectNetwork */, \
  0x00000008 /* ReorderNetwork */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   server_generated */ \
  0x00000001 /* ScanNetworksResponse */, \
  0x00000007 /* ConnectNetworkResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Diagnostic Logs (server) */\
  /*   client_generated */ \
  0x00000000 /* RetrieveLogsRequest */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   server_generated */ \
  0x00000001 /* RetrieveLogsResponse */, \
  chip::kInvalidCommandId /* end of list */, \
  /* Endpoint: 0, Cluster: Operational Credentials (server) */\
  /*   client_generated */ \
  0x00000000 /* AttestationRequest */, \
  0x00000002 /* CertificateChainRequest */, \
  0x00000004 /* CSRRequest */, \
  0x00000006 /* AddNOC */, \
  0x00000009 /* UpdateFabricLabel */, \
  0x0000000A /* RemoveFabric */, \
  0x0000000B /* AddTrustedRootCertificate */, \
  chip::kInvalidCommandId /* end of list */, \
  /*   server_generated */ \
  0x00000001 /* AttestationResponse */, \
  0x00000003 /* CertificateChainResponse */, \
  0x00000005 /* CSRResponse */, \
  0x00000008 /* NOCResponse */, \
  chip::kInvalidCommandId /* end of list */, \
}

// clang-format on

#define ZAP_CLUSTER_MASK(mask) CLUSTER_MASK_##mask
#define GENERATED_CLUSTER_COUNT 6

// clang-format off
#define GENERATED_CLUSTERS { \
  { \
      /* Endpoint: 0, Cluster: Access Control (server) */ \
      .clusterId = 0x0000001F,  \
      .attributes = ZAP_ATTRIBUTE_INDEX(0), \
      .attributeCount = 4, \
      .clusterSize = 2, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .clientGeneratedCommandList = nullptr ,\
      .serverGeneratedCommandList = nullptr ,\
    },\
  { \
      /* Endpoint: 0, Cluster: General Commissioning (server) */ \
      .clusterId = 0x00000030,  \
      .attributes = ZAP_ATTRIBUTE_INDEX(4), \
      .attributeCount = 3, \
      .clusterSize = 10, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .clientGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 0 ) ,\
      .serverGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 4 ) ,\
    },\
  { \
      /* Endpoint: 0, Cluster: Network Commissioning (server) */ \
      .clusterId = 0x00000031,  \
      .attributes = ZAP_ATTRIBUTE_INDEX(7), \
      .attributeCount = 1, \
      .clusterSize = 2, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .clientGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 7 ) ,\
      .serverGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 14 ) ,\
    },\
  { \
      /* Endpoint: 0, Cluster: Diagnostic Logs (client) */ \
      .clusterId = 0x00000032,  \
      .attributes = ZAP_ATTRIBUTE_INDEX(8), \
      .attributeCount = 0, \
      .clusterSize = 0, \
      .mask = ZAP_CLUSTER_MASK(CLIENT), \
      .functions = NULL, \
      .clientGeneratedCommandList = nullptr ,\
      .serverGeneratedCommandList = nullptr ,\
    },\
  { \
      /* Endpoint: 0, Cluster: Diagnostic Logs (server) */ \
      .clusterId = 0x00000032,  \
      .attributes = ZAP_ATTRIBUTE_INDEX(8), \
      .attributeCount = 0, \
      .clusterSize = 0, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .clientGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 17 ) ,\
      .serverGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 19 ) ,\
    },\
  { \
      /* Endpoint: 0, Cluster: Operational Credentials (server) */ \
      .clusterId = 0x0000003E,  \
      .attributes = ZAP_ATTRIBUTE_INDEX(8), \
      .attributeCount = 5, \
      .clusterSize = 4, \
      .mask = ZAP_CLUSTER_MASK(SERVER), \
      .functions = NULL, \
      .clientGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 21 ) ,\
      .serverGeneratedCommandList = ZAP_GENERATED_COMMANDS_INDEX( 29 ) ,\
    },\
}

// clang-format on

#define ZAP_CLUSTER_INDEX(index) (&generatedClusters[index])

#define ZAP_FIXED_ENDPOINT_DATA_VERSION_COUNT 5

// This is an array of EmberAfEndpointType structures.
#define GENERATED_ENDPOINT_TYPES                                                                                                   \
    {                                                                                                                              \
        { ZAP_CLUSTER_INDEX(0), 6, 18 },                                                                                           \
    }

// Largest attribute size is needed for various buffers
#define ATTRIBUTE_LARGEST (9)

static_assert(ATTRIBUTE_LARGEST <= CHIP_CONFIG_MAX_ATTRIBUTE_STORE_ELEMENT_SIZE, "ATTRIBUTE_LARGEST larger than expected");

// Total size of singleton attributes
#define ATTRIBUTE_SINGLETONS_SIZE (0)

// Total size of attribute storage
#define ATTRIBUTE_MAX_SIZE (18)

// Number of fixed endpoints
#define FIXED_ENDPOINT_COUNT (1)

// Array of endpoints that are supported, the data inside
// the array is the endpoint number.
#define FIXED_ENDPOINT_ARRAY                                                                                                       \
    {                                                                                                                              \
        0x0000                                                                                                                     \
    }

// Array of profile ids
#define FIXED_PROFILE_IDS                                                                                                          \
    {                                                                                                                              \
        0x0256                                                                                                                     \
    }

// Array of device ids
#define FIXED_DEVICE_IDS                                                                                                           \
    {                                                                                                                              \
    }

// Array of device versions
#define FIXED_DEVICE_VERSIONS                                                                                                      \
    {                                                                                                                              \
        1                                                                                                                          \
    }

// Array of endpoint types supported on each endpoint
#define FIXED_ENDPOINT_TYPES                                                                                                       \
    {                                                                                                                              \
        0                                                                                                                          \
    }

// Array of networks supported on each endpoint
#define FIXED_NETWORKS                                                                                                             \
    {                                                                                                                              \
        0                                                                                                                          \
    }
