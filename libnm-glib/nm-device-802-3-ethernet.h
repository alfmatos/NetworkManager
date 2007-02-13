#ifndef NM_DEVICE_802_3_ETHERNET_H
#define NM_DEVICE_802_3_ETHERNET_H

#include "nm-device.h"

#define NM_TYPE_DEVICE_802_3_ETHERNET            (nm_device_802_3_ethernet_get_type ())
#define NM_DEVICE_802_3_ETHERNET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_DEVICE_802_3_ETHERNET, NMDevice8023Ethernet))
#define NM_DEVICE_802_3_ETHERNET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_DEVICE_802_3_ETHERNET, NMDevice8023EthernetClass))
#define NM_IS_DEVICE_802_3_ETHERNET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_DEVICE_802_3_ETHERNET))
#define NM_IS_DEVICE_802_3_ETHERNET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_DEVICE_802_3_ETHERNET))
#define NM_DEVICE_802_3_ETHERNET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_DEVICE_802_3_ETHERNET, NMDevice8023EthernetClass))

typedef struct {
	NMDevice parent;
} NMDevice8023Ethernet;

typedef struct {
	NMDeviceClass parent;
} NMDevice8023EthernetClass;

GType nm_device_802_3_ethernet_get_type (void);

NMDevice8023Ethernet *nm_device_802_3_ethernet_new         (DBusGConnection *connection,
															const char *path);
int                   nm_device_802_3_ethernet_get_speed   (NMDevice8023Ethernet *device);
char                 *nm_device_802_3_ethernet_get_address (NMDevice8023Ethernet *device);
void                  nm_device_802_3_ethernet_activate    (NMDevice8023Ethernet *device,
															gboolean user_requested);

#endif /* NM_DEVICE_802_3_ETHERNET_H */