#include "nm-object.h"
#include "NetworkManager.h"

G_DEFINE_ABSTRACT_TYPE (NMObject, nm_object, G_TYPE_OBJECT)

#define NM_OBJECT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NM_TYPE_OBJECT, NMObjectPrivate))

typedef struct {
	DBusGConnection *connection;
	char *path;
	DBusGProxy *properties_proxy;

	gboolean disposed;
} NMObjectPrivate;

enum {
	PROP_0,
	PROP_CONNECTION,
	PROP_PATH,

	LAST_PROP
};

static void
nm_object_init (NMObject *object)
{
}

static GObject*
constructor (GType type,
			 guint n_construct_params,
			 GObjectConstructParam *construct_params)
{
	GObject *object;
	NMObjectPrivate *priv;

	object = G_OBJECT_CLASS (nm_object_parent_class)->constructor (type,
																   n_construct_params,
																   construct_params);
	if (!object)
		return NULL;

	priv = NM_OBJECT_GET_PRIVATE (object);

	if (priv->connection == NULL || priv->path == NULL) {
		g_warning ("Connection or path not received.");
		g_object_unref (object);
		return NULL;
	}

	priv->properties_proxy = dbus_g_proxy_new_for_name (priv->connection,
														NM_DBUS_SERVICE,
														priv->path,
														"org.freedesktop.DBus.Properties");

	return object;
}

static void
dispose (GObject *object)
{
	NMObjectPrivate *priv = NM_OBJECT_GET_PRIVATE (object);

	if (priv->disposed)
		return;

	priv->disposed = TRUE;

	g_object_unref (priv->properties_proxy);
	dbus_g_connection_unref (priv->connection);

	G_OBJECT_CLASS (nm_object_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	NMObjectPrivate *priv = NM_OBJECT_GET_PRIVATE (object);

	g_free (priv->path);

	G_OBJECT_CLASS (nm_object_parent_class)->finalize (object);
}

static void
set_property (GObject *object, guint prop_id,
			  const GValue *value, GParamSpec *pspec)
{
	NMObjectPrivate *priv = NM_OBJECT_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_CONNECTION:
		/* Construct only */
		priv->connection = dbus_g_connection_ref ((DBusGConnection *) g_value_get_boxed (value));
		break;
	case PROP_PATH:
		/* Construct only */
		priv->path = g_strdup (g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object, guint prop_id,
			  GValue *value, GParamSpec *pspec)
{
	NMObjectPrivate *priv = NM_OBJECT_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_CONNECTION:
		g_value_set_boxed (value, priv->connection);
		break;
	case PROP_PATH:
		g_value_set_string (value, priv->path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nm_object_class_init (NMObjectClass *nm_object_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (nm_object_class);

	g_type_class_add_private (nm_object_class, sizeof (NMObjectPrivate));

	/* virtual methods */
	object_class->constructor = constructor;
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	/* porperties */
	g_object_class_install_property
		(object_class, PROP_CONNECTION,
		 g_param_spec_boxed (NM_OBJECT_CONNECTION,
							 "Connection",
							 "Connection",
							 DBUS_TYPE_G_CONNECTION,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class, PROP_PATH,
		 g_param_spec_string (NM_OBJECT_PATH,
							  "Object Path",
							  "DBus Object Path",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

DBusGConnection *
nm_object_get_connection (NMObject *object)
{
	g_return_val_if_fail (NM_IS_OBJECT (object), NULL);

	return NM_OBJECT_GET_PRIVATE (object)->connection;
}

const char *
nm_object_get_path (NMObject *object)
{
	g_return_val_if_fail (NM_IS_OBJECT (object), NULL);

	return NM_OBJECT_GET_PRIVATE (object)->path;
}


gboolean
nm_object_get_property (NMObject *object,
						const char *interface,
						const char *prop_name,
						GValue *value)
{
	GError *err = NULL;

	g_return_val_if_fail (NM_IS_OBJECT (object), FALSE);
	g_return_val_if_fail (interface != NULL, FALSE);
	g_return_val_if_fail (prop_name != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	if (!dbus_g_proxy_call (NM_OBJECT_GET_PRIVATE (object)->properties_proxy,
							"Get", &err,
							G_TYPE_STRING, interface,
							G_TYPE_STRING, prop_name,
							G_TYPE_INVALID,
							G_TYPE_VALUE, value,
							G_TYPE_INVALID)) {
		g_warning ("Error in get_property: %s\n", err->message);
		g_error_free (err);
		return FALSE;
	}

	return TRUE;
}

void
nm_object_set_property (NMObject *object,
						const char *interface,
						const char *prop_name,
						GValue *value)
{
	g_return_if_fail (NM_IS_OBJECT (object));
	g_return_if_fail (interface != NULL);
	g_return_if_fail (prop_name != NULL);
	g_return_if_fail (G_IS_VALUE (value));

	dbus_g_proxy_call_no_reply (NM_OBJECT_GET_PRIVATE (object)->properties_proxy,
								"Set",
								G_TYPE_STRING, interface,
								G_TYPE_STRING, prop_name,
								G_TYPE_VALUE, value,
								G_TYPE_INVALID);
}

char *
nm_object_get_string_property (NMObject *object,
							   const char *interface,
							   const char *prop_name)
{
	char *str = NULL;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value))
		str = g_strdup (g_value_get_string (&value));

	return str;
}

char *
nm_object_get_object_path_property (NMObject *object,
									const char *interface,
									const char *prop_name)
{
	char *path = NULL;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value))
		path = g_strdup (g_value_get_boxed (&value));

	return path;
}

gint32
nm_object_get_int_property (NMObject *object,
							const char *interface,
							const char *prop_name)
{
	gint32 i = 0;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value))
		i = g_value_get_int (&value);

	return i;
}

guint32
nm_object_get_uint_property (NMObject *object,
							 const char *interface,
							 const char *prop_name)
{
	guint32 i = 0;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value))
		i = g_value_get_uint (&value);

	return i;
}

gboolean
nm_object_get_boolean_property (NMObject *object,
								const char *interface,
								const char *prop_name)
{
	gboolean b;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value))
		b = g_value_get_boolean (&value);

	return b;
}

gdouble
nm_object_get_double_property (NMObject *object,
							   const char *interface,
							   const char *prop_name)
{
	gdouble d;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value))
		d = g_value_get_double (&value);

	return d;
}

GByteArray *
nm_object_get_byte_array_property (NMObject *object,
								   const char *interface,
								   const char *prop_name)
{
	GByteArray * array = NULL;
	GValue value = {0,};

	if (nm_object_get_property (object, interface, prop_name, &value)) {
		GArray * tmp = g_value_get_boxed (&value);
		int i;
		unsigned char byte;

		array = g_byte_array_sized_new (tmp->len);
		for (i = 0; i < tmp->len; i++) {
			byte = g_array_index (tmp, unsigned char, i);
			g_byte_array_append (array, &byte, 1);
		}
	}

	return array;
}
