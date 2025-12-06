/*
 *
 * Copyright (C) 2019, Northern Digital Inc. All rights reserved.
 *
 * All Northern Digital Inc. ("NDI") Media and/or Sample Code and/or Sample Code
 * Documentation (collectively referred to as "Sample Code") is licensed and provided "as
 * is" without warranty of any kind. The licensee, by use of the Sample Code, warrants to
 * NDI that the Sample Code is fit for the use and purpose for which the licensee intends to
 * use the Sample Code. NDI makes no warranties, express or implied, that the functions
 * contained in the Sample Code will meet the licensee's requirements or that the operation
 * of the programs contained therein will be error free. This warranty as expressed herein is
 * exclusive and NDI expressly disclaims any and all express and/or implied, in fact or in
 * law, warranties, representations, and conditions of every kind pertaining in any way to
 * the Sample Code licensed and provided by NDI hereunder, including without limitation,
 * each warranty and/or condition of quality, merchantability, description, operation,
 * adequacy, suitability, fitness for particular purpose, title, interference with use or
 * enjoyment, and/or non infringement, whether express or implied by statute, common law,
 * usage of trade, course of dealing, custom, or otherwise. No NDI dealer, distributor, agent
 * or employee is authorized to make any modification or addition to this warranty.
 * In no event shall NDI nor any of its employees be liable for any direct, indirect,
 * incidental, special, exemplary, or consequential damages, sundry damages or any
 * damages whatsoever, including, but not limited to, procurement of substitute goods or
 * services, loss of use, data or profits, or business interruption, however caused. In no
 * event shall NDI's liability to the licensee exceed the amount paid by the licensee for the
 * Sample Code or any NDI products that accompany the Sample Code. The said limitations
 * and exclusions of liability shall apply whether or not any such damages are construed as
 * arising from a breach of a representation, warranty, guarantee, covenant, obligation,
 * condition or fundamental term or on any theory of liability, whether in contract, strict
 * liability, or tort (including negligence or otherwise) arising in any way out of the use of
 * the Sample Code even if advised of the possibility of such damage. In no event shall
 * NDI be liable for any claims, losses, damages, judgments, costs, awards, expenses or
 * liabilities of any kind whatsoever arising directly or indirectly from any injury to person
 * or property, arising from the Sample Code or any use thereof
 *
 */

/**
 * demo program illustrating the use of the NDI GStreamer elements,
 * but not using the trackbin, all the pipeline elements are
 * created here.
 *
 */

#include <stdio.h>
#include <gst/gst.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gst/gstdebugutils.h>
#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#define DO_LOCAL_TRACKSRC_LOAD 1

#if DO_LOCAL_TRACKSRC_LOAD

#include "trackoverlay.h"
#include "tracksrc.h"

#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <unistd.h>
#endif

int folder_exists(const char *folder_path) {
    return access(folder_path, F_OK) == 0;
}

int create_folder(const char *folder_path) {
    int result = 0;

    // Check if the folder already exists
    if (!folder_exists(folder_path)) {
        // Create the folder
        #ifdef _WIN32
            result = mkdir(folder_path);
        #else
            result = mkdir(folder_path, 0777); // 0777 sets read, write, and execute permissions for owner, group, and others
        #endif

        if (result == 0) {
            printf("Folder created successfully: %s\n", folder_path);
        } else {
            printf("Failed to create folder: %s\n", folder_path);
        }
    } else {
        printf("Folder already exists: %s\n", folder_path);
    }

    return result;
}


static int RTSP_PROTOCOL_FLAG = 0;
static int VIDEO_LATENCY = 200;

typedef struct _NDIVideoPipeline NDIVideoPipeline;

typedef struct {
  GtkWidget *sink_widget;
} GUIData;

struct _NDIVideoPipeline
{
  GMainLoop *loop;
  GstBus *bus;
  GstElement *pipeline;
  GstElement *klv_bin;
  GstElement *rtsp_source;
  GstElement *video_bin, *video_bin2;
  GstElement *video_display;
  GstElement *gtkglsinkbin;
  GstElement *tee;
  GstElement *save_frame_bin;
  GstElement *track_source;
  GstElement *track_queue;
  GstElement *track_overlay;
  GIOChannel *io_stdin;
  GstPad *overlay_video_sink;
  GstPad *overlay_video_src;
  gboolean klv_chain_linked;
  gboolean h264_chain_linked;
  GUIData *gui_data;
};


void startVideoStream (char *connect_to, char *tool_directory, char *tool_file, char *rtspsrc_location, char* rtsp_video_port, char *capture_path);

int main (int argc, char *argv[])
{
  if (argc != 7)
  {
    /* example usage:
     *
     * ardemo.exe "P9-00311.local" "c:/capisample/sroms/" "8700339.rom" "rtsp://169.254.99.8:554/video" "554"
     *
     * the arguments are: (1) device to connect to
     *                    (2) tool directory
     *                    (3) tool file
     *                    (4) rtspsrc with port
     *                    (5) rtsp video port
     */
    printf ("Wrong number of parameters, Usage is\n");
    printf ("> ardemo.exe \"P9-00311.local\" \"c:/capisample/sroms/\" \"8700339.rom\" \"rtsp://169.254.99.8:554/video\" \"554\" \"path/to/capture data\"\n");
    printf ("... Hit any key to quit\n");
    getchar ();
    return -1;
  }
  printf ("... Starting NDI GStreamer sample.  Hit Enter key to quit\n");
  gst_init (NULL, NULL);
  gtk_init (NULL, NULL);
#if DO_LOCAL_TRACKSRC_LOAD
  register_local_plugin_track_overlay ();
  register_local_plugin_target_src ();
#endif

  gchar *capture_path = g_strdelimit(argv[6], "\"", ' ');
  create_folder(capture_path);
  startVideoStream (argv[1], argv[2], argv[3], argv[4], argv[5], capture_path);
}

static void showGSVersion ();
static gboolean track_target_init (NDIVideoPipeline *pipeline_data, char *connect_to, char *tool_directory, char *tool_file, char *video_port, char *capture_path);
static gboolean stateChangedCB (GstBus *bus, GstMessage *msg, gpointer data);
static void eosCB (GstBus *bus, GstMessage *msg, gpointer *data);
static void warningCB (GstBus *bus, GstMessage *msg, gpointer *data);
static void errorCB (GstBus *bus, GstMessage *msg, gpointer *data);
static void pad_added_handler (GstElement *src, GstPad *new_pad, gpointer *data);
static gboolean handle_keyboard (GIOChannel *source, GIOCondition cond, gpointer user_data);
static void create_ui (NDIVideoPipeline *pipeline_data);


void create_label_and_entry(GtkWidget *parent, const char *label_text, GtkWidget **entry) {
    GtkWidget *label = gtk_label_new(label_text);
    *entry = gtk_entry_new();

    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(parent), container);
    gtk_box_pack_start(GTK_BOX(container), label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(container), *entry, FALSE, FALSE, 10);
}


void offset_button_handler(GtkWidget *widget, gpointer data)
{
  NDIVideoPipeline *pipeline_data = (NDIVideoPipeline *) data;
  GtkWidget *dialog, *label, *content_area;
  GtkWidget *x_entry, *y_entry, *z_entry;
  GtkWidget *hbox;
  GtkDialogFlags flags;
  gchar *tool_files = "";
  g_object_get(pipeline_data->track_source, "tool-file", &tool_files, NULL);
  gchar **tool_files_split = g_strsplit(tool_files, " ", -1);
  // Create the popup window
  flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  dialog = gtk_dialog_new_with_buttons("Offset Position",
                     NULL,
                     flags,
                     ("_OK"),
                     GTK_RESPONSE_ACCEPT,
                     ("_Cancel"),
                     GTK_RESPONSE_REJECT,
                     NULL);

  gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 120);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  // Create a horizontal box
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // Create entries for x, y, z coordinates
  x_entry = gtk_entry_new();
  y_entry = gtk_entry_new();
  z_entry = gtk_entry_new();

  // Add labels and entries to the box
  label = gtk_label_new("x :");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), x_entry, TRUE, TRUE, 0);

  label = gtk_label_new("y :");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), y_entry, TRUE, TRUE, 0);

  label = gtk_label_new("z :");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), z_entry, TRUE, TRUE, 0);

  GtkWidget *radio_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

  label = gtk_label_new("Select the tool:");
  gtk_box_pack_start(GTK_BOX(radio_hbox), label, FALSE, FALSE, 10);

  GtkWidget *radio_group_member = NULL;

  for (gchar **ptr = tool_files_split; *ptr; ptr++)
  {
    GtkWidget *radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_group_member), *ptr);
    gtk_box_pack_start(GTK_BOX(radio_hbox), radio_button, FALSE, FALSE, 10);
    radio_group_member = radio_button;
  }

  gtk_box_pack_start(GTK_BOX(content_area), radio_hbox, FALSE, FALSE, 10);

  // Add the box to the dialog
  gtk_container_add(GTK_CONTAINER(content_area), hbox);

  gtk_widget_show_all(dialog);

  // Handle the response
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  switch (result)
  {
    case GTK_RESPONSE_ACCEPT:
       // User clicked OK, get the offset position
       const gchar *x_text = gtk_entry_get_text(GTK_ENTRY(x_entry));
       const gchar *y_text = gtk_entry_get_text(GTK_ENTRY(y_entry));
       const gchar *z_text = gtk_entry_get_text(GTK_ENTRY(z_entry));

      GList *children = gtk_container_get_children(GTK_CONTAINER(radio_hbox));
      gint index = 0;
      for (GList *l = children; l != NULL; l = l->next)
      {
        GtkWidget *child = GTK_WIDGET(l->data);
        if (GTK_IS_RADIO_BUTTON(child) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(child)))
        {
          break;
        }
        index++;
      }
      g_list_free(children);

       gchar *offset_string = g_strdup_printf("%s,%s,%s,%d", x_text, y_text, z_text, index-1);
       g_print("offset_string: %s\n", offset_string);
       g_object_set(pipeline_data->track_overlay, "apply-offset", offset_string, NULL);
       break;
    default:
       break;
  }

  gtk_widget_destroy(dialog);
}

void draw_marker_handler(GtkWidget *widget, gpointer data)
{
  NDIVideoPipeline *pipeline_data = (NDIVideoPipeline *) data;
  g_object_set(pipeline_data->track_overlay, "draw-markers", TRUE, NULL);
}

static void create_ui(NDIVideoPipeline *pipeline_data){
  GUIData *gui_data = pipeline_data->gui_data;

    // Create main window
    GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_fullscreen(GTK_WINDOW(main_window));

    // Create main vertical box
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_window), main_vbox);

    // Create horizontal box for video and buttons
    GtkWidget *video_buttons_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), video_buttons_hbox, TRUE, TRUE, 0);

    GtkWidget *main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start (GTK_BOX (main_hbox), gui_data->sink_widget, TRUE, TRUE, 0);

    gtk_widget_set_size_request(main_hbox, 540, 480);
    gtk_box_pack_start(GTK_BOX(video_buttons_hbox), main_hbox, FALSE, FALSE, 0);

    // Create buttons for the video
    GtkWidget *button_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *button1 = gtk_button_new_with_label("Show/Stop overlay");
    g_signal_connect(button1, "clicked", G_CALLBACK(draw_marker_handler), pipeline_data);
    GtkWidget *button2 = gtk_button_new_with_label("Apply Offset");
    g_signal_connect(button2, "clicked", G_CALLBACK(offset_button_handler), pipeline_data);
    gtk_widget_set_size_request(button_vbox, 400, 480);
    gtk_box_pack_start(GTK_BOX(button_vbox), button1, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(button_vbox), button2, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(video_buttons_hbox), button_vbox, FALSE, FALSE, 20);

    // Show all widgets
    gtk_widget_show_all(main_window);
}

void startVideoStream (char *connect_to, char *tool_directory, char *tool_file, char *rtspsrc_location, char* rtsp_video_port, char* capture_path)
{
  GError *e = NULL;
  NDIVideoPipeline pipeline_data = { 0 };
  guint outval = 0;
  GstStateChangeReturn ret;
  char video_bin_desc[256];
  char save_frame_bin_desc[256];
  GError *err = NULL;

  showGSVersion ();

  pipeline_data.loop = g_main_loop_new (NULL, FALSE);

  pipeline_data.pipeline = gst_element_factory_make ("pipeline", NULL);
  if (!pipeline_data.pipeline)
  {
    g_printerr ("Unable to create video pipeline\n");
    goto exit;
  }
  g_snprintf (video_bin_desc, sizeof (video_bin_desc), "rtspsrc location=%s latency=%d buffer-mode=%d name=VegaRTSPSource", rtspsrc_location, VIDEO_LATENCY, RTSP_PROTOCOL_FLAG);
  pipeline_data.rtsp_source = gst_parse_launch (video_bin_desc, &err);
  if (!pipeline_data.rtsp_source)
  {
    g_print ("invalid pipeline: %s\n", err->message);
    g_clear_error (&err);
    goto exit;
  }

  pipeline_data.video_bin = gst_parse_bin_from_description ("queue name=h264Queue ! rtph264depay ! h264parse ! avdec_h264 name=h264Decode", TRUE, &e);
  if (e || !pipeline_data.video_bin)
  {
    g_printerr ("Unable to create video bin\n");
    goto exit;
  }
  gst_element_set_name (pipeline_data.video_bin, "H264VideoBin");
  pipeline_data.video_bin2 = gst_parse_bin_from_description("queue name=H264Queue2 ! rtph264depay ! h264parse ! avdec_h264 name=H264Decode2", TRUE, &e);
  if (e || !pipeline_data.video_bin2)
  {
    g_printerr ("Unable to create video bin2\n");
    goto exit;
  }
  gst_element_set_name (pipeline_data.video_bin2, "H264VideoBin2");

  pipeline_data.tee = gst_element_factory_make ("tee", "GSTtee");

  if(!pipeline_data.tee)
  {
    g_printerr ("Unable to create tee\n");
    goto exit;
  }
  create_folder(g_strconcat(capture_path, "frames", NULL));
  g_snprintf(save_frame_bin_desc, sizeof (save_frame_bin_desc), "jpegenc ! multifilesink location=\"%sframes/frame_%%05d.jpg\"", capture_path);
  g_print("save_frame_bin_desc: %s\n", save_frame_bin_desc);
  pipeline_data.save_frame_bin = gst_parse_bin_from_description (save_frame_bin_desc, TRUE, &e);
  if (e || !pipeline_data.save_frame_bin)
  {
    g_printerr ("Unable to create save frame bin : %s\n", e->message);
    goto exit;
  }
  gst_element_set_name (pipeline_data.save_frame_bin, "SaveFrameBin");

  pipeline_data.video_display = gst_element_factory_make ("gtksink", "VideoDisplay");

  if(!pipeline_data.video_display)
  {
    g_printerr("Unable to create video display GTK bin \n");
    goto exit;
  }
  
  g_object_set (pipeline_data.video_display, "sync", 0, NULL);
  pipeline_data.gui_data = g_new0 (GUIData, 1); 
  g_object_get (pipeline_data.video_display, "widget", &pipeline_data.gui_data->sink_widget, NULL);

  track_target_init (&pipeline_data, connect_to, tool_directory, tool_file, rtsp_video_port, capture_path);

  pipeline_data.klv_bin = gst_parse_bin_from_description ("queue name=RTPKLVQueue ! rtpklvdepay name=RTPKLVDepay", TRUE, &e);
  if (e || !pipeline_data.klv_bin)
  {
    g_printerr ("Unable to create klv bin\n");
    goto exit;
  }
  gst_element_set_name (pipeline_data.klv_bin, "KLVTimeStampBin");

  /* add the other elements to the pipeline */
  gst_bin_add_many (GST_BIN (pipeline_data.pipeline), pipeline_data.rtsp_source, pipeline_data.video_bin, pipeline_data.klv_bin, 
                    pipeline_data.save_frame_bin, pipeline_data.tee, pipeline_data.video_bin2, NULL);
  // pipeline_data.overlay_video_sink = gst_element_get_static_pad(pipeline_data.save_frame_bin, "sink");

  /* If the top-level object is not a pipeline, place it in a pipeline. */
  if (!GST_IS_PIPELINE (pipeline_data.pipeline)) {
    g_print ("top-level object is not a pipeline");
    goto exit;
  }
  create_ui(&pipeline_data);

  /* Wait until error or EOS */
  pipeline_data.bus = gst_element_get_bus (pipeline_data.pipeline);
  gst_bus_add_signal_watch (pipeline_data.bus);
  /* use the sync signal handler to link elements while the pipeline is still
   * doing the state change */
  gst_bus_set_sync_handler (pipeline_data.bus, gst_bus_sync_signal_handler, &pipeline_data, NULL);
  g_object_connect (pipeline_data.bus, "signal::sync-message::error", (GCallback) errorCB, &pipeline_data, NULL);
  g_signal_connect (pipeline_data.bus, "message::warning", (GCallback) warningCB, &pipeline_data);
  g_signal_connect (pipeline_data.bus, "message::state-changed", G_CALLBACK (stateChangedCB), &pipeline_data);
  g_signal_connect (pipeline_data.bus, "message::eos", (GCallback) eosCB, &pipeline_data);
  /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
  pipeline_data.io_stdin = g_io_channel_win32_new_fd (_fileno (stdin));
#else
  pipeline_data.io_stdin = g_io_channel_unix_new (STDIN_FILENO);
#endif
  outval = g_io_add_watch (pipeline_data.io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, &pipeline_data);

  /* Connect to the pad-added signal */
  g_signal_connect (pipeline_data.rtsp_source, "pad-added", G_CALLBACK (pad_added_handler), &pipeline_data);

  /* Start playing */
  ret = gst_element_set_state (pipeline_data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    goto exit;
  }
  g_print ("Pipeline started\n");
  g_main_loop_run (pipeline_data.loop);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline_data.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "NDIVideoPipeline");
  gst_element_set_state (pipeline_data.pipeline, GST_STATE_NULL);
  g_print ("Pipeline stopped\n");

exit:
  if (pipeline_data.loop)
    g_main_loop_unref (pipeline_data.loop);
  if (pipeline_data.pipeline)
    gst_object_unref (pipeline_data.pipeline);
  if (pipeline_data.bus)
  {
    gst_bus_remove_signal_watch (pipeline_data.bus);
    gst_object_unref (pipeline_data.bus);
  }
  return;
}

static void showGSVersion ()
{
  const gchar *nano_str;
  guint major, minor, micro, nano;

  gst_version (&major, &minor, &micro, &nano);
  nano_str = nano == 1 ? "(CVS)" : nano == 2 ? "(Prerelease)" : "";
  g_print ("Initialized GStreamer %d.%d.%d %s\n", major, minor, micro, nano_str);
}

static gboolean track_target_init (NDIVideoPipeline *pipeline_data, char *connect_to, char *tool_directory, char *tool_file, char *video_port, char *capture_path)
{
  gboolean target_chain_ok = FALSE;
  pipeline_data->track_source = gst_element_factory_make ("tracksrc", "NDItrackSource");
  g_object_set (pipeline_data->track_source, "connect-to", connect_to, NULL);
  g_object_set (pipeline_data->track_source, "tool-location", tool_directory, NULL);
  g_object_set (pipeline_data->track_source, "tool-file", tool_file, NULL);
  g_object_set (pipeline_data->track_source, "rtsp-port", video_port, NULL);
  pipeline_data->track_queue = gst_element_factory_make ("queue", "TrackQueue");
  pipeline_data->track_overlay = gst_element_factory_make ("trackoverlay", "NDITrackOverlay");
  g_object_set (pipeline_data->track_overlay, "capture-path", capture_path, NULL);
  GstElement *autoVideoConvert = gst_element_factory_make ("autovideoconvert", "AutoVideoConvert");
  // GstElement *fakesink = gst_element_factory_make("fakesink", "fakesink");

  gst_bin_add_many (GST_BIN (pipeline_data->pipeline), pipeline_data->track_queue, pipeline_data->track_source, 
                    pipeline_data->track_overlay, autoVideoConvert, pipeline_data->video_display, NULL);
  if (gst_element_link_many (pipeline_data->track_source, pipeline_data->track_queue, pipeline_data->track_overlay, autoVideoConvert,pipeline_data->video_display , NULL))
  {
    g_print ("Track branch linked OK\n");
    target_chain_ok = TRUE;
  }
  else
  {
    g_print ("Track branch NOT linked OK\n");
    target_chain_ok = TRUE;
  }
  
  pipeline_data->overlay_video_sink = gst_element_get_static_pad (pipeline_data->track_overlay, "sink");

  return target_chain_ok;
}

static gboolean isRTPSrcPadEncodingType (GstPad *pad, const char *encoding_type)
{
  GstCaps *pad_caps = NULL;
  GstStructure *pad_struct = NULL;
  const gchar *pad_type = NULL;
  const gchar *pad_name = NULL;
  const gchar *encoding = NULL;
  gboolean isRtp = FALSE;
  gboolean is_needed_type = FALSE;

  pad_caps = gst_pad_get_current_caps (pad);
  if (!pad_caps || gst_caps_is_any (pad_caps) || gst_caps_is_empty (pad_caps))
  {
    g_print ("caps is not KLV -failed first check\n");
    goto exit;
  }

  pad_name = gst_object_get_name (GST_OBJECT_CAST (pad));
  if (!g_str_match_string ("recv_rtp_src", pad_name, 0))
  {
    g_print ("pad caps - name is does not contain recv_rtp_src\n");
    goto exit;
  }

  pad_struct = gst_caps_get_structure (pad_caps, 0);
  pad_type = gst_structure_get_name (pad_struct);

  isRtp = g_str_has_prefix (pad_type, "application/x-rtp");
  if (!isRtp)
  {
    g_print ("caps is not rtp - failed second check\n");
    goto exit;
  }

  encoding = gst_structure_get_string (pad_struct, "encoding-name");
  if (!g_str_match_string (encoding_type, encoding, 0))
  {
    g_print ("pad caps - encoding is NOT %s\n", encoding_type);
    goto exit;
  }
  else
  {
    g_print ("pad caps - encoding IS %s\n", encoding_type);
  }
  is_needed_type = TRUE;

exit:
  /* Unreference the new pad's caps, if we got them */
  if (pad_caps != NULL)
    gst_caps_unref (pad_caps);

  return is_needed_type;
}

static gboolean stateChangedCB (GstBus *bus, GstMessage *msg, gpointer data)
{
  NDIVideoPipeline *ndiVideo = (NDIVideoPipeline *) data;
  GstState old_state, new_state, pending_state;

  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  g_print ("Element %s changed state from %s to %s.\n", GST_OBJECT_NAME (msg->src),
    gst_element_state_get_name (old_state),
    gst_element_state_get_name (new_state));

  if (ndiVideo->klv_chain_linked && ndiVideo->h264_chain_linked && new_state == GST_STATE_PLAYING && GST_ELEMENT_CAST (msg->src) == ndiVideo->rtsp_source)
  {
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (ndiVideo->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "NDIVideoPipelinePlaying");
  }
  return TRUE;
}

/* This function is called when an warning message is posted on the bus */
static void warningCB (GstBus *bus, GstMessage *msg, gpointer *data)
{
  GError *err;
  gchar *debug_info;
  gst_message_parse_warning (msg, &err, &debug_info);
  NDIVideoPipeline *ndiVideo = (NDIVideoPipeline *) data;
  /* Print details on the screen */
  g_print ("Warning received from element %s: %s Debug Info: %s\n", GST_OBJECT_NAME (msg->src), err->message, debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);
}

/* This function is called when an error message is posted on the bus */
static void errorCB (GstBus *bus, GstMessage *msg, gpointer *data)
{
  GError *err;
  gchar *debug_info;
  static gboolean done_once = FALSE;

  gst_message_parse_error (msg, &err, &debug_info);
  NDIVideoPipeline *ndiVideo = (NDIVideoPipeline *) data;
  /* Print error details on the screen */
 	g_printerr("Error received from element %s: %s Debug Info: %s\n", GST_OBJECT_NAME(msg->src), err->message, debug_info ? debug_info : "none");
  if (!done_once)
  {
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (ndiVideo->pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "NDIVideoPipelineError");
    done_once = TRUE;
  }
  g_clear_error (&err);
  g_free (debug_info);
  g_main_loop_quit(ndiVideo->loop);
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eosCB (GstBus *bus, GstMessage *msg, gpointer *data)
{
  g_print ("End-Of-Stream reached.\n");
  NDIVideoPipeline *ndiVideo = (NDIVideoPipeline *) data;
  g_main_loop_quit(ndiVideo->loop);
}

/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, gpointer *data)
{
  NDIVideoPipeline *ndiVideo = (NDIVideoPipeline *) data;
  GstPadLinkReturn ret = GST_PAD_LINK_OK;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  if (isRTPSrcPadEncodingType (new_pad, "SMPTE336M"))
  {
    if (ndiVideo->klv_chain_linked)
    {
      g_print ("klv already linked. Ignoring.\n");
      goto exit;
    }
    if (gst_element_link_many (ndiVideo->rtsp_source, ndiVideo->klv_bin, NULL))
    {
      ndiVideo->klv_chain_linked = TRUE;
      g_print ("KLV source linked OK\n");
    }
    else
    {
      g_print ("Error KLV source NOT linked\n");
    }

    if (gst_element_link_many (ndiVideo->klv_bin, ndiVideo->track_source, NULL))
    {
      g_print ("linked OK\n");
    }
    else
    {
      g_print ("linked NOT OK\n");
    }
  }
  else if (isRTPSrcPadEncodingType (new_pad, "H264"))
  {
    if (ndiVideo->h264_chain_linked)
    {
      g_print ("h264 already linked. Ignoring.\n");
      goto exit;
    }

    

    if (gst_element_link_many (ndiVideo->rtsp_source, ndiVideo->tee, NULL))
    {
      ndiVideo->h264_chain_linked = TRUE;
      g_print ("video bin linked to source OK\n");
    }
    else
    {
      g_print ("Error video bin linked to source OK\n");
    }

    GstPad *tee_pad_overlay = gst_element_request_pad_simple (ndiVideo->tee, "src_%u");
    GstPad *queue_pad = gst_element_get_static_pad(ndiVideo->video_bin, "sink");
    GstPad *tee_pad_save = gst_element_request_pad_simple (ndiVideo->tee, "src_%u");
    GstPad *save_queue = gst_element_get_static_pad(ndiVideo->video_bin2, "sink");

    if(gst_pad_link (tee_pad_overlay, queue_pad) != GST_PAD_LINK_OK  || gst_pad_link(tee_pad_save, save_queue) != GST_PAD_LINK_OK)
    {
      g_print ("tee pad NOT linked\n");
    }
    else
    {
      g_print ("tee pad linked OK\n");
    }

    ret = gst_pad_link ((GstPad *) ndiVideo->video_bin->srcpads->data, ndiVideo->overlay_video_sink);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("linked NOT OK\n");
    }
    else {
      g_print ("linked OK\n");
    }

    if(gst_pad_link((GstPad *) ndiVideo->video_bin2->srcpads->data, gst_element_get_static_pad(ndiVideo->save_frame_bin, "sink")) == GST_PAD_LINK_OK){
      g_print ("linked OK\n");
    }
    else{
      g_print ("linked NOT OK\n");
    }
  }

exit:
  g_print ("Exit pad_added_handler\n");
}

/* Process keyboard input */
static gboolean handle_keyboard (GIOChannel *source, GIOCondition cond, gpointer user_data) {
  gchar *str = NULL;
  NDIVideoPipeline *ndiVideo = (NDIVideoPipeline *) user_data;

  if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) {
    return TRUE;
  }
  g_print ("Key pressed %s\n", str);

  switch (g_ascii_tolower (str[0])) {
    case '0':
      gst_element_set_state (ndiVideo->pipeline, GST_STATE_NULL);
      break;
    case 'p':
      gst_element_set_state (ndiVideo->pipeline, GST_STATE_PAUSED);
      break;
    case 'r':
      gst_element_set_state (ndiVideo->pipeline, GST_STATE_PLAYING);
      break;
    case 'd':
      g_object_set(ndiVideo->track_overlay, "draw-markers", FALSE, NULL);
      break;
    default:
      gst_element_send_event (ndiVideo->pipeline, gst_event_new_eos ());
      g_main_loop_quit (ndiVideo->loop);
      break;
  }
  g_free (str);
  return TRUE;
}