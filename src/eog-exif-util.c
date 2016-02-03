/* Eye Of Gnome - EXIF Utilities
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 * Author: Felix Riemann <felix@hsgheli.de>
 *
 * Based on code by:
 *	- Jens Finke <jens@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRPTIME
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#include <time.h>

#include "eog-exif-util.h"
#include "eog-util.h"

#include <string.h>
#include <glib/gi18n.h>

#define DATE_BUF_SIZE 200

/* gboolean <-> gpointer conversion macros taken from gedit */
#ifndef GBOOLEAN_TO_POINTER
#define GBOOLEAN_TO_POINTER(i) (GINT_TO_POINTER ((i) ? 2 : 1))
#endif
#ifndef GPOINTER_TO_BOOLEAN
#define GPOINTER_TO_BOOLEAN(i) ((gboolean) ((GPOINTER_TO_INT (i) == 2) ? TRUE : FALSE))
#endif

typedef ExifData EogExifData;

/* Define EogExifData type */
G_DEFINE_BOXED_TYPE(EogExifData, eog_exif_data, eog_exif_data_copy, eog_exif_data_free)

static gpointer
_check_strptime_updates_wday (gpointer data)
{
	struct tm tm;

	memset (&tm, '\0', sizeof (tm));
	strptime ("2008:12:24 20:30:45", "%Y:%m:%d %T", &tm);
	/* Check if tm.tm_wday is set to Wednesday (3) now */
	return GBOOLEAN_TO_POINTER (tm.tm_wday == 3);
}

/**
 * _calculate_wday_yday:
 * @tm: A struct tm that should be updated.
 *
 * Ensure tm_wday and tm_yday are set correctly in a struct tm.
 * The other date (dmy) values must have been set already.
 **/
static void
_calculate_wday_yday (struct tm *tm)
{
	GDate *exif_date;
	struct tm tmp_tm;

	exif_date = g_date_new_dmy (tm->tm_mday,
				    tm->tm_mon+1,
				    tm->tm_year+1900);

	g_return_if_fail (exif_date != NULL && g_date_valid (exif_date));

	// Use this to get GLib <-> libc corrected values
	g_date_to_struct_tm (exif_date, &tmp_tm);
	g_date_free (exif_date);

	tm->tm_wday = tmp_tm.tm_wday;
	tm->tm_yday = tmp_tm.tm_yday;
}

#ifdef HAVE_STRPTIME
static gchar *
eog_exif_util_format_date_with_strptime (const gchar *date)
{
	static GOnce strptime_updates_wday = G_ONCE_INIT;
	gchar *new_date = NULL;
	gchar tmp_date[DATE_BUF_SIZE];
	gchar *p;
	gsize dlen;
	struct tm tm;

	memset (&tm, '\0', sizeof (tm));
	p = strptime (date, "%Y:%m:%d %T", &tm);

	if (p == date + strlen (date)) {
		g_once (&strptime_updates_wday,
			_check_strptime_updates_wday,
			NULL);

		// Ensure tm.tm_wday and tm.tm_yday are set
		if (!GPOINTER_TO_BOOLEAN (strptime_updates_wday.retval))
			_calculate_wday_yday (&tm);

		/* A strftime-formatted string, to display the date the image was taken.  */
		dlen = strftime (tmp_date, DATE_BUF_SIZE * sizeof(gchar), _("%a, %d %B %Y  %X"), &tm);
		new_date = g_strndup (tmp_date, dlen);
	}

	return new_date;
}
#else
static gchar *
eog_exif_util_format_date_by_hand (const gchar *date)
{
	int year, month, day, hour, minutes, seconds;
	int result;
	gchar *new_date = NULL;

	result = sscanf (date, "%d:%d:%d %d:%d:%d",
			 &year, &month, &day, &hour, &minutes, &seconds);

	if (result < 3 || !g_date_valid_dmy (day, month, year)) {
		return NULL;
	} else {
		gchar tmp_date[DATE_BUF_SIZE];
		gsize dlen;
		time_t secs;
		struct tm tm;

		memset (&tm, '\0', sizeof (tm));
		tm.tm_mday = day;
		tm.tm_mon = month-1;
		tm.tm_year = year-1900;
		// Calculate tm.tm_wday
		_calculate_wday_yday (&tm);

		if (result < 5) {
		  	/* A strftime-formatted string, to display the date the image was taken, for the case we don't have the time.  */
			dlen = strftime (tmp_date, DATE_BUF_SIZE * sizeof(gchar), _("%a, %d %B %Y"), &tm);
		} else {
			tm.tm_sec = result < 6 ? 0 : seconds;
			tm.tm_min = minutes;
			tm.tm_hour = hour;
			/* A strftime-formatted string, to display the date the image was taken.  */
			dlen = strftime (tmp_date, DATE_BUF_SIZE * sizeof(gchar), _("%a, %d %B %Y  %X"), &tm);
		}

		if (dlen == 0)
			return NULL;
		else
			new_date = g_strndup (tmp_date, dlen);
	}
	return new_date;
}
#endif /* HAVE_STRPTIME */

/**
 * eog_exif_util_format_date:
 * @date: a date string following Exif specifications
 *
 * Takes a date string formatted after Exif specifications and generates a
 * more readable, possibly localized, string out of it.
 *
 * Returns: a newly allocated date string formatted according to the
 * current locale.
 */
gchar *
eog_exif_util_format_date (const gchar *date)
{
	gchar *new_date;
#ifdef HAVE_STRPTIME
	new_date = eog_exif_util_format_date_with_strptime (date);
#else
	new_date = eog_exif_util_format_date_by_hand (date);
#endif /* HAVE_STRPTIME */
	return new_date;
}

void
eog_exif_util_set_label_text (GtkLabel *label,
			      EogExifData *exif_data,
			      gint tag_id)
{
	gchar exif_buffer[512];
	const gchar *buf_ptr;
	gchar *label_text = NULL;

	g_return_if_fail (GTK_IS_LABEL (label));

	if (exif_data) {
		buf_ptr = eog_exif_data_get_value (exif_data, tag_id,
						   exif_buffer, 512);

		if (tag_id == EXIF_TAG_DATE_TIME_ORIGINAL && buf_ptr)
			label_text = eog_exif_util_format_date (buf_ptr);
		else
			label_text = eog_util_make_valid_utf8 (buf_ptr);
	}

	gtk_label_set_text (label, label_text);
	g_free (label_text);
}

void
eog_exif_util_set_focal_length_label_text (GtkLabel *label,
					   EogExifData *exif_data)
{
	ExifEntry *entry = NULL, *entry35mm = NULL;
	ExifByteOrder byte_order;
	gfloat f_val = 0.0;
	gchar *fl_text = NULL,*fl35_text = NULL;

	/* If no ExifData is supplied the label will be
	 * cleared later as fl35_text is NULL. */
	if (exif_data != NULL) {
		entry = exif_data_get_entry (exif_data, EXIF_TAG_FOCAL_LENGTH);
		entry35mm = exif_data_get_entry (exif_data,
					    EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM);
		byte_order = exif_data_get_byte_order (exif_data);
	}

	if (entry && G_LIKELY (entry->format == EXIF_FORMAT_RATIONAL)) {
		ExifRational value;

		/* Decode value by hand as libexif is not necessarily returning
		 * it in the format we want it to be.
		 */
		value = exif_get_rational (entry->data, byte_order);
		/* Guard against div by zero */
		if (G_LIKELY(value.denominator != 0))
			f_val = (gfloat)value.numerator/
				(gfloat)value.denominator;

		/* TRANSLATORS: This is the actual focal length used when
		   the image was taken.*/
		fl_text = g_strdup_printf (_("%.1f (lens)"), f_val);

	}
	if (entry35mm && G_LIKELY (entry35mm->format == EXIF_FORMAT_SHORT)) {
		ExifShort s_val;

		s_val = exif_get_short (entry35mm->data, byte_order);

		/* Print as float to get a similar look as above. */
		/* TRANSLATORS: This is the equivalent focal length assuming
		   a 35mm film camera. */
		fl35_text = g_strdup_printf(_("%.1f (35mm film)"),(float)s_val);
	}

	if (fl_text) {
		if (fl35_text) {
			gchar *merged_txt;

			merged_txt = g_strconcat (fl35_text,", ", fl_text, NULL);
			gtk_label_set_text (label, merged_txt);
			g_free (merged_txt);
		} else {
			gtk_label_set_text (label, fl_text);
		}
	} else {
		/* This will also clear the label if no ExifData was supplied */
		gtk_label_set_text (label, fl35_text);
	}

	g_free (fl35_text);
	g_free (fl_text);
}

/**
 * eog_exif_data_get_value:
 * @exif_data: pointer to an <structname>ExifData</structname> struct
 * @tag_id: the requested tag's id. See <filename>exif-tag.h</filename>
 * from the libexif package for possible values (e.g. %EXIF_TAG_EXPOSURE_MODE).
 * @buffer: a pre-allocated output buffer
 * @buf_size: size of @buffer
 *
 * Convenience function to extract a string representation of an Exif tag
 * directly from an <structname>ExifData</structname> struct. The string is
 * written into @buffer as far as @buf_size permits.
 *
 * Returns: a pointer to @buffer.
 */
const gchar *
eog_exif_data_get_value (EogExifData *exif_data, gint tag_id, gchar *buffer, guint buf_size)
{
	ExifEntry *exif_entry;
	const gchar *exif_value;

        exif_entry = exif_data_get_entry (exif_data, tag_id);

	buffer[0] = 0;

	exif_value = exif_entry_get_value (exif_entry, buffer, buf_size);

	return exif_value;
}

EogExifData *
eog_exif_data_copy (EogExifData *data)
{
	exif_data_ref (data);

	return data;
}

void
eog_exif_data_free (EogExifData *data)
{
	exif_data_unref (data);
}
