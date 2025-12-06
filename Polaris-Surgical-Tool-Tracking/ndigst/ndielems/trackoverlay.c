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
 * SECTION:element-trackoverlay
 * @title: trackoverlay
 * @see_also: tracksrc, trackbin, #GstToolTracker, #GstLensParam
 *
 * The trackoverlay has two sink pads, one to receive tracking data  
 * and the other to receive video data. For each video buffer, the 
 * matching PTS of the track data is found (if any).  If matching 
 * track data is found, it is transformed to video space, and track 
 * overlays are drawn on the video corresponding to the tool marker 
 * positions.  This is an augmented reality type application.
 *
 * The transformation parameters are either set with the 
 * transform-params parameter, or with a GST_EVENT_CUSTOM_DOWNSTREAM 
 * event of type GstNDILensParamsSet.  
 
 * The frequency of the tracking data may be different from the video
 * frame rate, and this tracking frequency is set with the track-freq 
 * parameter, or with a GST_EVENT_CUSTOM_DOWNSTREAM event of type 
 * GstNDITrackFreqSet.  
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 trackbin connect-to=P9-00311.local rtsp-port=554 tool-location=c:/capisample/sroms/ tool-file=8700339.rom ! autovideosink sync=false
 * ]|
 * in this case, the trackoverlay element is managed within the trackbin element
 * </refsect2>
 *
 */


#include "config.h"
#include "trackoverlay.h"
#include "commonutils.h"

/* TODO: also need origin overlay definition (and drawing) */

#define OVERLAY_WIDTH 15
#define OVERLAY_HEIGHT 15
#define OFFSET_X 7
#define OFFSET_Y 7
#define CLIP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

/* OpenCV 	Mat mat(15, 15, CV_8UC4); with red circle of radius 7 */
const guchar red_circle_BGRA[] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96,
  0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 255, 96, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0,
  0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96,
  0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0,
  0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255,
  96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0,
  255, 96, 0, 0, 255, 96, 0, 0, 255, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 96, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0
};

static GstBuffer *overlay_buf = NULL;
char *camera_par_file = "";
char *capture_path = "";
FILE *tool_fps_6d = NULL;
FILE *track_fps_3d = NULL;
FILE *track_fps_2d = NULL;
FILE *video_fps = NULL;
static gboolean default_draw_marker = false;
static Quat offset[MAX_NUM_MARKERS] = { 0 };

#define VIDEO_FORMATS "{ RGBx, RGB, BGR, BGRx, xRGB, xBGR, " \
    "RGBA, BGRA, ARGB, ABGR, I420, YV12, AYUV, YUY2, UYVY, " \
    "v308, v210, v216, Y41B, Y42B, Y444, YVYU, NV12, NV21, UYVP, " \
    "RGB16, BGR16, RGB15, BGR15, UYVP, A420, YUV9, YVU9, " \
    "IYU1, ARGB64, AYUV64, r210, I420_10LE, I420_10BE, " \
    "GRAY8, GRAY16_BE, GRAY16_LE }"

GST_DEBUG_CATEGORY_STATIC (track_overlay_debug);
#define GST_CAT_DEFAULT track_overlay_debug

enum {
  PROP_0,
  TRANSFORM_PARAM,
  TRACK_FREQ,
  DRAW_MARKERS,
  CAPTURE_PATH,
  APPLY_OFFSET,
  N_PROPERTIES,
};

static GParamSpec *klass_properties[N_PROPERTIES] = { NULL, };

#define gst_track_overlay_parent_class parent_class
G_DEFINE_TYPE (GstTrackOverlay, gst_track_overlay, GST_TYPE_VIDEO_FILTER);

static void gst_track_overlay_finalize (GObject * object);

static size_t get_track_marker_count (GstBuffer *buffer, GstTrackOverlay *elem);
static void prepare_track_data (GstBuffer *buffer, GstTrackOverlay *elem);

static GstStaticPadTemplate gst_track_overlay_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (VIDEO_FORMATS))
);

static GstStaticPadTemplate gst_track_overlay_sink_pad_template = GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (VIDEO_FORMATS))
);

static GstStaticPadTemplate gst_track_overlay_tracksink_pad_template =
GST_STATIC_PAD_TEMPLATE ("tracksink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("trackdata"));

static gboolean gst_ndi_track_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_ndi_track_data_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static gboolean gst_track_overlay_start (GstBaseTransform * trans);
static gboolean gst_track_overlay_stop (GstBaseTransform * bt);
static GstFlowReturn gst_track_overlay_transform_frame_ip (GstVideoFilter
  * trans, GstVideoFrame * frame);
static void gst_track_overlay_before_transform (GstBaseTransform * trans,
  GstBuffer * outbuf);
static GstFlowReturn
gst_track_overlay_submit_input_buffer (GstBaseTransform * trans,
  gboolean is_discont, GstBuffer * input);
static gboolean gst_overly_set_info (GstVideoFilter * trans,
  GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
  GstVideoInfo * out_info);
static void set_marker_match_toler (GstTrackOverlay *overlay_elem);
static void gst_track_overlay_set_property (GObject * object, guint prop_id,
  const GValue * value, GParamSpec * pspec);
static void gst_track_overlay_get_property (GObject * object, guint prop_id,
  GValue * value, GParamSpec * pspec);

static gboolean
gst_track_overlay_stop (GstBaseTransform * bt)
{
  GstTrackOverlay *overlay_elem = GST_TRACK_OVERLAY (bt);

  GST_OBJECT_LOCK (overlay_elem);

  fclose(track_fps_2d);
  fclose(tool_fps_6d);
  fclose(track_fps_3d);
  fclose(video_fps);
  if (overlay_elem->comp) {
    gst_video_overlay_composition_unref (overlay_elem->comp);
    overlay_elem->comp = NULL;
  }
  gst_data_queue_flush (overlay_elem->queue);

  GST_OBJECT_UNLOCK (overlay_elem);
  return TRUE;
}

static void
gst_track_overlay_class_init (GstTrackOverlayClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoFilterClass *vfilter_class = GST_VIDEO_FILTER_CLASS (klass);
  GstBaseTransformClass *bt_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  vfilter_class->set_info = gst_overly_set_info;

  bt_class->start = GST_DEBUG_FUNCPTR (gst_track_overlay_start);
  vfilter_class->transform_frame_ip =
    gst_track_overlay_transform_frame_ip;
  bt_class->transform_ip_on_passthrough = TRUE;
  bt_class->stop = gst_track_overlay_stop;
  bt_class->before_transform =
    GST_DEBUG_FUNCPTR (gst_track_overlay_before_transform);
  bt_class->submit_input_buffer =
    gst_track_overlay_submit_input_buffer;

  gst_element_class_add_static_pad_template (element_class,
    &gst_track_overlay_src_pad_template);
  gst_element_class_add_static_pad_template (element_class,
    &gst_track_overlay_tracksink_pad_template);
  gst_element_class_add_static_pad_template (element_class,
    &gst_track_overlay_sink_pad_template);
  gobject_class->set_property = gst_track_overlay_set_property;
  gobject_class->get_property = gst_track_overlay_get_property;
  gobject_class->finalize = gst_track_overlay_finalize;

  klass_properties[TRANSFORM_PARAM] = g_param_spec_object ("transform-params", "Lens Parameters", "Lens Parameters",
    G_TYPE_OBJECT, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  klass_properties[TRACK_FREQ] = g_param_spec_int ("track-freq", "Track Frequency",
    "Tool Update Frequency (HZ)", 0, G_MAXINT32, DEFAULT_TRACK_FREQUENCY,
    (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

  klass_properties[DRAW_MARKERS] = g_param_spec_boolean ("draw-markers", "Draw Markers",
    "Show Markers on the Video", default_draw_marker, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

  klass_properties[CAPTURE_PATH] = g_param_spec_string ("capture-path", "Capture Path",
    "Path to capture the track data", "", (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS) );

  klass_properties[APPLY_OFFSET] = g_param_spec_string ("apply-offset", "Apply Offset",
    "Apply Offset to the markers", "", (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_properties (gobject_class, N_PROPERTIES,
    klass_properties);

  gst_element_class_set_metadata (element_class,
    "Tracking data overlay drawing", "Filter/Converter",
    "Draws tracking points to screen space",
    "NDI");
  GST_DEBUG_CATEGORY_INIT (track_overlay_debug, "trackoverlay", 0, "NDI Tracking Overlay");
}

/** 
 * Sets the transformation information used to project the 3D tracking 
 * points to video space
 *
 */
static void set_projection_params (GstTrackOverlay * overlay_elem)
{
  GstLensParam *gst_lens = NULL;
  FILE *fp = fopen(camera_par_file, "w");
  fprintf(fp, "{\n" );
  gst_lens = overlay_elem->gst_lens_param;
  ProjectToScreenParams *project = &overlay_elem->project_points;
  LensParams *ndi_params = &project->ndi_lens_params;
  double zhang[NO_OF_ZHANG_PARAMS] = { gst_lens->k1 , gst_lens->k2, gst_lens->k3, gst_lens->p1, gst_lens->p2 };
  fprintf(fp, "\"Distortions\" : {\"k1\": %f, \"k2\": %f, \"k3\": %f, \"p1\": %f, \"p2\": %f},\n", gst_lens->k1, gst_lens->k2, gst_lens->k3, gst_lens->p1, gst_lens->p2);
  project->distort[0] = gst_lens->k1;
  project->distort[1] = gst_lens->k2;
  project->distort[4] = gst_lens->k3;
  project->distort[2] = gst_lens->p1;
  project->distort[3] = gst_lens->p2;
  CameraParametersMat (project->native_cam, gst_lens->u0, gst_lens->v0, gst_lens->fu, gst_lens->fv);
  ndi_params->res_char.binX = gst_lens->binX;
  ndi_params->res_char.binY = gst_lens->binY;
  ndi_params->res_char.left = gst_lens->left;
  ndi_params->res_char.top = gst_lens->top;
  ndi_params->vcu_align.q0 = gst_lens->q0;
  ndi_params->vcu_align.qx = gst_lens->qx;
  ndi_params->vcu_align.qy = gst_lens->qy;
  ndi_params->vcu_align.qz = gst_lens->qz;
  ndi_params->vcu_trans.tx = gst_lens->tx;
  ndi_params->vcu_trans.ty = gst_lens->ty;
  ndi_params->vcu_trans.tz = gst_lens->tz;
  fprintf(fp, "\"Rotational\" : {\"q0\" : %f, \"qx\" : %f, \"qy\" : %f, \"qz\" : %f},\n", gst_lens->q0, gst_lens->qx, gst_lens->qy, gst_lens->qz);
  fprintf(fp, "\"Translation\" : {\"tx\" : %f, \"ty\" : %f, \"tz\" : %f},\n", gst_lens->tx, gst_lens->ty, gst_lens->tz);
  CreateCameraParametersMatFromNative (project->native_cam, project->cam_param, &ndi_params->res_char);
  fprintf(fp, "\"CameraMatrix\" : [[%f, %f, %f], [%f, %f, %f], [%f, %f, %f]]\n", 
            project->cam_param[0][0], project->cam_param[0][1], project->cam_param[0][2],
            project->cam_param[1][0], project->cam_param[1][1], project->cam_param[1][2],
            project->cam_param[2][0], project->cam_param[2][1], project->cam_param[2][2]);
  fprintf(fp, "}\n");
  fclose(fp);
}

static void
gst_track_overlay_set_property (GObject * object, guint prop_id,
  const GValue * value, GParamSpec * pspec)
{
  GstTrackOverlay *overlay_elem;
  overlay_elem = GST_TRACK_OVERLAY (object);

  switch (prop_id) {
    case TRANSFORM_PARAM:
      if (overlay_elem->gst_lens_param)
        g_object_unref (overlay_elem->gst_lens_param);
      overlay_elem->gst_lens_param = GST_LENS_PARAM (g_value_get_object (value));
      if (overlay_elem->gst_lens_param)
      {
        g_object_ref (overlay_elem->gst_lens_param);
        set_projection_params (overlay_elem);
      }
      break;
    case TRACK_FREQ:
      overlay_elem->track_frequency = g_value_get_int (value);
      set_marker_match_toler (overlay_elem);
      break;
    case DRAW_MARKERS:
      default_draw_marker = !default_draw_marker;
      break;
    case CAPTURE_PATH:
      capture_path = g_value_dup_string(value);
      char *tool_data_6d = g_strconcat(capture_path, "tool_data_6d.csv", NULL);
      char *track_data_3d = g_strconcat(capture_path, "track_data_3d.csv", NULL);
      char *track_data_2d = g_strconcat(capture_path, "track_data_2d.csv", NULL);
      char *video_pts_file = g_strconcat(capture_path, "video_pts.csv", NULL);
      camera_par_file = g_strconcat(capture_path, "camera_par.json", NULL);
      tool_fps_6d = fopen(tool_data_6d, "w");
      track_fps_3d = fopen(track_data_3d, "w");
      track_fps_2d = fopen(track_data_2d, "w");
      video_fps = fopen(video_pts_file, "w");
      break;
    case APPLY_OFFSET:
      gchar *offset_str = g_value_dup_string(value);
      gchar **offset_str_split = g_strsplit(offset_str, ",", 0);
      gint tool = atoi(offset_str_split[3]);
      offset[tool].qx = atof(offset_str_split[0]);
      offset[tool].qy = atof(offset_str_split[1]);
      offset[tool].qz = atof(offset_str_split[2]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Potential TODO:

   An code update would need to be added to set individual parameters from the command prompt (gst-launch-1.0)
   For example:
   GstStructure *props;
   props = gst_structure_from_string("props,q0= (double)0.55,qx= (double)0.42", NULL);
   g_object_set(pipeline_data->track_overlay, "transform-params", props, NULL);
   gst_structure_free(props);

   This would allow us not to rely on the live source to get the transformation parameters if we can use recorded track data.
*/

static void
gst_track_overlay_get_property (GObject * object, guint prop_id,
  GValue * value, GParamSpec * pspec)
{
  GstTrackOverlay *overlay_elem;
  overlay_elem = GST_TRACK_OVERLAY (object);

  switch (prop_id) {
    case TRANSFORM_PARAM:
      g_value_set_object (value, overlay_elem->gst_lens_param);
      break;
    case TRACK_FREQ:
      g_value_set_int (value, overlay_elem->track_frequency);
      break;
    case DRAW_MARKERS:
      g_value_set_boolean (value, overlay_elem->do_draw_markers);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_track_overlay_finalize (GObject * object)
{
  GstTrackOverlay *overlay_elem;

  overlay_elem = GST_TRACK_OVERLAY (object);
  if (overlay_elem->gst_lens_param) {
    g_object_unref (overlay_elem->gst_lens_param);
  }
  g_clear_object (&overlay_elem->queue);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
data_queue_item_free (GstDataQueueItem * item)
{
  GST_DEBUG ("release  %p", item->object);
  gst_buffer_unref (GST_BUFFER (item->object));
  g_slice_free (GstDataQueueItem, item);
}

/**
 * creates the overlay buffer that represent a marker position
 * (currently a coloured circle)
 */
static GstBuffer *
create_overlay_buffer (GstTrackOverlay * overlay, const guchar *overlay_primitive)
{
  GstBuffer *buffer;
  GstVideoInfo info;
  GstVideoFrame frame;
  guint8 * data;
  gint x, y, saved_data_index;

  gst_video_info_set_format (&info, GST_VIDEO_FORMAT_BGRA, OVERLAY_WIDTH, OVERLAY_HEIGHT);
  buffer = gst_buffer_new_and_alloc (info.size);
  gst_buffer_add_video_meta (buffer, GST_VIDEO_FRAME_FLAG_NONE,
    GST_VIDEO_INFO_FORMAT (&info),
    GST_VIDEO_INFO_WIDTH (&info),
    GST_VIDEO_INFO_HEIGHT (&info));
  gst_video_frame_map (&frame, &info, buffer, GST_MAP_WRITE);
  saved_data_index = 0;
  data = (guint8 *) GST_VIDEO_FRAME_PLANE_DATA (&frame, 0);

  for (y = 0; y < OVERLAY_WIDTH; y++) {
    guint8 *line = &data[y * GST_VIDEO_FRAME_PLANE_STRIDE (&frame, 0)];
    for (x = 0; x < OVERLAY_HEIGHT; x++) {
      guint8 *pixel = &line[x * 4];
      pixel[0] = overlay_primitive[saved_data_index++];
      pixel[1] = overlay_primitive[saved_data_index++];
      pixel[2] = overlay_primitive[saved_data_index++];
      pixel[3] = overlay_primitive[saved_data_index++];
    }
  }

  gst_video_frame_unmap (&frame);

  return buffer;
}

static gboolean
data_queue_check_full (GstDataQueue * queue, guint visible,
  guint bytes, guint64 time, gpointer checkdata)
{
  return FALSE;
}

/**
 * sets the tolerance used to choose the match of the track data to the 
 * video data. The tolerance depends on the track data frequency
 */
static void set_marker_match_toler (GstTrackOverlay *overlay_elem)
{
  gfloat frame_delta_ms = (gfloat) (1.0 / overlay_elem->track_frequency * 1000.);
  /* testing shows choosing a marker time stamp biased towards the previous frame results in a better match */
  /* hardware engineers say maybe because of the exposure time of the video */
  overlay_elem->match_marker_low_ms = (gint) frame_delta_ms + 1;
  overlay_elem->match_marker_high_ms = (gint) frame_delta_ms * 2 + 2;
  GST_DEBUG_OBJECT (overlay_elem, "Setting track frequency to %d, sample interval %f ms, match_marker_low %d ms, match_marker_high %d ms",
    overlay_elem->track_frequency, frame_delta_ms, overlay_elem->match_marker_low_ms, overlay_elem->match_marker_high_ms);
}

static void
gst_track_overlay_init (GstTrackOverlay * overlay_elem)
{
  overlay_buf = create_overlay_buffer (overlay_elem, red_circle_BGRA);
  overlay_elem->queue = gst_data_queue_new (data_queue_check_full, NULL, NULL, NULL);
  overlay_elem->trackpad = gst_pad_new_from_static_template (&gst_track_overlay_tracksink_pad_template, "tracksink");
  gst_pad_set_event_function (overlay_elem->trackpad,
    GST_DEBUG_FUNCPTR (gst_ndi_track_sink_event));
  gst_element_add_pad (GST_ELEMENT (overlay_elem), overlay_elem->trackpad);
  gst_pad_set_chain_function (overlay_elem->trackpad,
    GST_DEBUG_FUNCPTR (gst_ndi_track_data_chain));
  overlay_elem->track_frequency = DEFAULT_TRACK_FREQUENCY;
  overlay_elem->do_draw_markers = default_draw_marker;
  set_marker_match_toler (overlay_elem);
}

/* this function handles sink events */
static gboolean
gst_ndi_track_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstTrackOverlay *overlay_elem;
  GstStructure *s;

  overlay_elem = GST_TRACK_OVERLAY (parent);

  GST_DEBUG_OBJECT (overlay_elem, "Received %s event: %" GST_PTR_FORMAT,
    GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_DOWNSTREAM:
    {
      /* lens paramaters sink event */
      if (gst_event_has_name (event, "GstNDILensParamsSet"))
      {
        GST_DEBUG_OBJECT (overlay_elem, "got GST_EVENT_CUSTOM_DOWNSTREAM,  GstNDILensParamsSet");

        event = gst_event_make_writable (event);
        s = gst_event_writable_structure (event);

        if (overlay_elem->gst_lens_param)
        {
          g_object_unref (overlay_elem->gst_lens_param);
        }
        if (gst_structure_has_field_typed (s, "LensParams", G_TYPE_OBJECT))
        {
          gst_structure_get (s, "LensParams", G_TYPE_OBJECT, &overlay_elem->gst_lens_param, NULL);
        }
        if (overlay_elem->gst_lens_param)
        {
          g_object_ref (overlay_elem->gst_lens_param);
          set_projection_params (overlay_elem);
        }
      }
      /* track data frequency sink event */
      else if (gst_event_has_name (event, "GstNDITrackFreqSet"))
      {
        GST_DEBUG_OBJECT (overlay_elem, "got GST_EVENT_CUSTOM_DOWNSTREAM,  GstNDITrackFreqSet");
        event = gst_event_make_writable (event);
        s = gst_event_writable_structure (event);
        if (gst_structure_has_field_typed (s, "TrackFrequency", G_TYPE_INT))
        {
          gint track_freq;
          gst_structure_get (s, "TrackFrequency", G_TYPE_INT, &track_freq, NULL);
          if (track_freq > 0)
          {
            overlay_elem->track_frequency = track_freq;
            set_marker_match_toler (overlay_elem);
          }
        }
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
}

/** 
 * creates the overlay of the marker locations to 
 * be drawn on the video buffer
 */
static GstVideoOverlayComposition *
create_overlay (GstTrackOverlay *overlay_elem, GstBuffer *buffer)
{
  GstVideoOverlayRectangle *rect;
  GstVideoOverlayComposition *comp = NULL;
  GstClockTime pts = GST_CLOCK_TIME_NONE;

  for (int i = 0; i < overlay_elem->marker_count; i++)
  {
    pts = overlay_elem->track_data_PTS;
    rect = gst_video_overlay_rectangle_new_raw (buffer,
      (gint) overlay_elem->markers[i].tx - OFFSET_X, (gint) overlay_elem->markers[i].ty - OFFSET_Y,
      OVERLAY_WIDTH, OVERLAY_HEIGHT,
      GST_VIDEO_OVERLAY_FORMAT_FLAG_NONE);
    if (comp == NULL)
    {
      comp = gst_video_overlay_composition_new (rect);
    }
    else
    {
      gst_video_overlay_composition_add_rectangle (comp, rect);
    }

    if (i == overlay_elem->marker_count - 1)
    {
      GST_DEBUG_OBJECT (overlay_elem, "Overlay for markers PTS %" GST_TIME_FORMAT ", num markers = %" G_GSIZE_FORMAT, GST_TIME_ARGS (pts), overlay_elem->marker_count);
    }

    gst_video_overlay_rectangle_unref (rect);
  }
  return comp;
}

/* chain function
 * for the track data
 */
static GstFlowReturn
gst_ndi_track_data_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstTrackOverlay *overlay_elem;
  gboolean queued = FALSE;
  GstDataQueueItem *item = NULL;
  GstClockTime track_pts = GST_CLOCK_TIME_NONE;

  overlay_elem = GST_TRACK_OVERLAY (parent);
  track_pts = GST_BUFFER_PTS (buf);
  if (get_track_marker_count (buf, overlay_elem) > 0)
  {
    item = g_slice_new (GstDataQueueItem);
    item->object = GST_MINI_OBJECT (buf);
    item->size = (guint) gst_buffer_get_size (buf);
    item->duration = GST_BUFFER_DURATION (buf);
    item->visible = TRUE;
    item->destroy = (GDestroyNotify) data_queue_item_free;

    GST_DEBUG_OBJECT (overlay_elem, "creating GstDataQueueItem");
    queued = gst_data_queue_push (overlay_elem->queue, item);
    if (item && !queued) {
      GST_DEBUG_OBJECT (overlay_elem, "could not add buffer to queue");
      /* Can't add buffer to queue. Must be flushing. */
      data_queue_item_free (item);
    }
    if (item && queued)
    {
      GST_DEBUG_OBJECT (overlay_elem, "Confirm added track data PTS %" GST_TIME_FORMAT " buffer to Queue", GST_TIME_ARGS (track_pts));
    }
  }
  else
  {
    GST_DEBUG_OBJECT (overlay_elem, "NOT Added track data PTS %" GST_TIME_FORMAT " has no markers, ignoring", GST_TIME_ARGS (track_pts));
    /* done with the track data */
    gst_buffer_unref (buf);
  }

  return GST_FLOW_OK;
}

static gboolean discard_queued_item (GstTrackOverlay *overlay_elem, GstDataQueueItem *item)
{
  gboolean discarded_ok = TRUE;
  GstDataQueueItem *item2 = NULL;
  GstDataQueueSize size;

  if (!gst_data_queue_pop (overlay_elem->queue, &item2) || item2 != item) {
    discarded_ok = FALSE;
    GST_WARNING_OBJECT (overlay_elem, "Unexpected Pop Result");
  }
  else
  {
    data_queue_item_free (item);
    gst_data_queue_get_level (overlay_elem->queue, &size);
    GST_DEBUG_OBJECT (overlay_elem, "discard_queued_item, Queue buffers size now: %d", size.visible);
  }
  return discarded_ok;
}

/**
 * finds the matching track data for the current video buffer based
 * on the PTS
 */
static gboolean find_matching_track_data (GstTrackOverlay *overlay_elem)
{
  GstDataQueueItem *item = NULL;
  GstDataQueueItem *item2 = NULL;
  GstBuffer *track_buffer;
  gboolean have_track_buffer = false;
  GstClockTime track_pts;
  GstClockTime time_diff;
  GstDataQueueSize size;
  gboolean look_for_match = true;

  while (look_for_match && !have_track_buffer)
  {
    look_for_match = false;
    item = NULL;
    gst_data_queue_get_level (overlay_elem->queue, &size);
    GST_DEBUG_OBJECT (overlay_elem, "Want match for Video PTS %" GST_TIME_FORMAT " Queue buffers size: %d", GST_TIME_ARGS (overlay_elem->video_data_PTS), size.visible);

    if (size.visible > 0)
    {
      GST_DEBUG_OBJECT (overlay_elem, "calling queue peek");
      if (!gst_data_queue_peek (overlay_elem->queue, &item))
      {
        GST_WARNING_OBJECT (overlay_elem, "problem peeking the queues data, leaving");
        break;
      }
    }
    else
    {
      GST_DEBUG_OBJECT (overlay_elem, "no queued data so leaving");
      break;
    }

    if (!item)
    {
      GST_WARNING_OBJECT (overlay_elem, "should not get here");
      break;
    }

    track_buffer = GST_BUFFER (item->object);
    track_pts = GST_BUFFER_PTS (track_buffer);
    GST_DEBUG_OBJECT (overlay_elem, "testing track data PTS %" GST_TIME_FORMAT " , video PTS %" GST_TIME_FORMAT, GST_TIME_ARGS (track_pts), GST_TIME_ARGS (overlay_elem->video_data_PTS));
    if (track_pts < overlay_elem->video_data_PTS)
    {
      time_diff = overlay_elem->video_data_PTS - track_pts;
      GST_DEBUG_OBJECT (overlay_elem, "have_track_buffer = FALSE (track PTS < video) TOO OLD, video is past this, throw away, diff %" GST_TIME_FORMAT,
        GST_TIME_ARGS (time_diff));
      look_for_match = discard_queued_item (overlay_elem, item);
    }
    else
    {
      time_diff = track_pts - overlay_elem->video_data_PTS;
      if (time_diff <= overlay_elem->match_marker_low_ms * (guint64) GST_MSECOND)
      {
        GST_DEBUG_OBJECT (overlay_elem, "have_track_buffer = FALSE (track PTS > video) but still TOO OLD, throw away, diff %" GST_TIME_FORMAT,
          GST_TIME_ARGS (time_diff));
        look_for_match = discard_queued_item (overlay_elem, item);
      }
      else if (time_diff <= overlay_elem->match_marker_high_ms * (guint64) GST_MSECOND)
      {
        have_track_buffer = true;
        fprintf(video_fps, ",%" GST_TIME_FORMAT, GST_TIME_ARGS(track_pts));
        GST_DEBUG_OBJECT (overlay_elem, "have_track_buffer = TRUE (track PTS > video), diff %" GST_TIME_FORMAT,
          GST_TIME_ARGS (time_diff));
      }
      else
      {
        /* note that other track data will have > PTS, so do not check anymore */
        GST_DEBUG_OBJECT (overlay_elem, "have_track_buffer = FALSE (track PTS > video) save for later match to video, diff %" GST_TIME_FORMAT,
          GST_TIME_ARGS (time_diff));
      }
    }

    if (have_track_buffer)
    {
      prepare_track_data (track_buffer, overlay_elem);
      discard_queued_item (overlay_elem, item);
    }
  }
  return have_track_buffer;
}

/** 
 * The before_transform implementation of the GstBaseTransformClass,
 * the overlay for drawing the marker positions is done at this time
 * 
 */
static void
gst_track_overlay_before_transform (GstBaseTransform * trans,
  GstBuffer * outbuf)
{
  GstTrackOverlay *overlay_elem;
  overlay_elem = GST_TRACK_OVERLAY (trans);
  gboolean found_match;
  fprintf(video_fps, "%" GST_TIME_FORMAT, GST_TIME_ARGS(overlay_elem->video_data_PTS));
  found_match = find_matching_track_data (overlay_elem);
  fprintf(video_fps, "\n");
  GST_DEBUG_OBJECT (overlay_elem, "found track data for video frame: %s", found_match ? "TRUE" : "FALSE");
  
  if (overlay_elem->comp) {
    gst_video_overlay_composition_unref (overlay_elem->comp);
    overlay_elem->comp = NULL;
  }

  if (overlay_elem->do_draw_markers)
  {
    GST_OBJECT_LOCK (overlay_elem);
    overlay_elem->comp = create_overlay (overlay_elem, overlay_buf);
    overlay_elem->do_draw_markers = FALSE;
    overlay_elem->marker_count = 0;
    GST_OBJECT_UNLOCK (overlay_elem);
  }
}

/* allows us to get the PTS of the video data buffer to be used later to match tracking data*/
static GstFlowReturn
gst_track_overlay_submit_input_buffer (GstBaseTransform * trans,
  gboolean is_discont, GstBuffer * input)
{
  GstTrackOverlay *overlay_elem;
  overlay_elem = GST_TRACK_OVERLAY (trans);
  GstFlowReturn ret;

  overlay_elem->video_data_PTS = input->pts;
  GST_DEBUG_OBJECT (overlay_elem, "track Overlay Video Buff PTS %" GST_TIME_FORMAT, GST_TIME_ARGS (input->pts));

  ret = GST_BASE_TRANSFORM_CLASS (parent_class)->submit_input_buffer (trans, is_discont, input);
  if (ret != GST_FLOW_OK || trans->queued_buf == NULL)
    return ret;

  if (!overlay_elem->got_first_buffer)
  {
    overlay_elem->first_buffer_PTS = input->pts;
    overlay_elem->got_first_buffer = true;
    GST_DEBUG_OBJECT (overlay_elem, "track Overlay Video Buff FIRST PTS %" GST_TIME_FORMAT, GST_TIME_ARGS (input->pts));
  }
  /* adjust the time stamps so as to start at the first buffer received */
  input->pts = input->pts - overlay_elem->first_buffer_PTS;

  overlay_elem->video_data_PTS = input->pts;
  GST_DEBUG_OBJECT (overlay_elem, "track Overlay Video Buff Adjusted PTS %" GST_TIME_FORMAT, GST_TIME_ARGS (input->pts));
  
  return ret;
}

/**
 * the transform_frame_ip implementation of the GstVideoFilterClass.
 * the video frame is transformed by painting the overlay on it
 */
static GstFlowReturn
gst_track_overlay_transform_frame_ip (GstVideoFilter * filter,
  GstVideoFrame * frame)
{
  GstTrackOverlay *overlay_elem;
  overlay_elem = GST_TRACK_OVERLAY (filter);

  if (overlay_elem->comp != NULL)
    gst_video_overlay_composition_blend (overlay_elem->comp, frame);

  GST_DEBUG_OBJECT (overlay_elem, "track Overlay Video Buff xform_ip PTS %" GST_TIME_FORMAT " DTS %" GST_TIME_FORMAT, GST_TIME_ARGS (frame->buffer->pts), GST_TIME_ARGS (frame->buffer->dts));
  return GST_FLOW_OK;
}

static gboolean gst_overly_set_info (GstVideoFilter * trans,
  GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
  GstVideoInfo * out_info)
{
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (trans), TRUE);
  return TRUE;
}

gboolean gst_track_overlay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "trackoverlay", GST_RANK_NONE,
    GST_TYPE_TRACK_OVERLAY);
}

static gboolean
gst_track_overlay_start (GstBaseTransform * trans)
{
  GstTrackOverlay *overlay_elem = GST_TRACK_OVERLAY (trans);
  gst_base_transform_set_passthrough (trans, TRUE);
  return TRUE;
}

static gboolean
gst_track_overlay_set_info (GstVideoFilter * filter, GstCaps * incaps,
  GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GST_INFO_OBJECT (filter, "caps: %" GST_PTR_FORMAT, incaps);
  return TRUE;
}


static size_t get_track_marker_count (GstBuffer *buffer, GstTrackOverlay *elem)
{
  void *track_data;
  GstMapInfo map;
  size_t num_markers = 0;
  Position marker_pos_buffer[MAX_NUM_MARKERS] = { 0 };
  bool buffer_too_small;
  GstClockTime track_data_PTS;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  track_data = (void *) map.data;
  track_data_PTS = buffer->pts;

  if (!track_data)
  {
    GST_DEBUG_OBJECT (elem, "track data is NULL");
    goto exit;
  }

  num_markers = GetMarkerPosFromBuffer (track_data, marker_pos_buffer, MAX_NUM_MARKERS, &buffer_too_small);
  for (size_t i = 0; i < num_markers; i++)
  {
    if (i == 0)
      GST_DEBUG_OBJECT (elem, "PTS Track data %" GST_TIME_FORMAT, GST_TIME_ARGS (track_data_PTS));
    GST_DEBUG_OBJECT (elem, "marker pos: %f %f %f", marker_pos_buffer[i].tx, marker_pos_buffer[i].ty, marker_pos_buffer[i].tz);
  }

exit:
  gst_buffer_unmap (buffer, &map);

  return num_markers;
}

Position find_point_at_distance(Position p1, Position p2, double distance)
{
  Position p3;
  double dx = p2.tx - p1.tx;
  double dy = p2.ty - p1.ty;
  double dz = p2.tz - p1.tz;
  double mag = sqrt(dx * dx + dy * dy + dz * dz);
  p3.tx = p1.tx + dx * distance / mag;
  p3.ty = p1.ty + dy * distance / mag;
  p3.tz = p1.tz + dz * distance / mag;
  return p3;
}

Position cross_product(Position p1, Position p2)
{
  Position p3;
  p3.tx = p1.ty * p2.tz - p1.tz * p2.ty;
  p3.ty = p1.tz * p2.tx - p1.tx * p2.tz;
  p3.tz = p1.tx * p2.ty - p1.ty * p2.tx;
  return p3;
}

Position find_plane_normal(Position p1, Position p2, Position p3)
{
  Position p4, p5;
  p4.tx = p2.tx - p1.tx;
  p4.ty = p2.ty - p1.ty;
  p4.tz = p2.tz - p1.tz;
  p5.tx = p3.tx - p1.tx;
  p5.ty = p3.ty - p1.ty;
  p5.tz = p3.tz - p1.tz;
  Position cross = cross_product(p4, p5);
  double mag = sqrt(cross.tx * cross.tx + cross.ty * cross.ty + cross.tz * cross.tz);
  cross.tx /= mag;
  cross.ty /= mag;
  cross.tz /= mag;
  return cross;
}

Quat hamilton_product(Quat q1, Quat q2) {
  Quat result;
  result.q0 = q1.q0*q2.q0 - q1.qx*q2.qx - q1.qy*q2.qy - q1.qz*q2.qz;
  result.qx = q1.q0*q2.qx + q1.qx*q2.q0 + q1.qy*q2.qz - q1.qz*q2.qy;
  result.qy = q1.q0*q2.qy - q1.qx*q2.qz + q1.qy*q2.q0 + q1.qz*q2.qx;
  result.qz = q1.q0*q2.qz + q1.qx*q2.qy - q1.qy*q2.qx + q1.qz*q2.q0;
  return result;
}

/**
 * transforms the track data to video space
 */
static void prepare_track_data (GstBuffer *buffer, GstTrackOverlay *elem)
{
  void *track_data;
  GstMapInfo map;
  size_t num_markers;
  Position tool_pos_buffer[MAX_NUM_MARKERS] = { 0 };
  Quat tool_quat_buffer[MAX_NUM_MARKERS] = { 0 };
  Position marker_pos_buffer[MAX_NUM_MARKERS] = { 0 };
  bool buffer_too_small;
  unsigned int frame_numbers[MAX_NUM_MARKERS] = { 0 };
  struct timespec time_specs[MAX_NUM_MARKERS] = { 0 };
  GstClockTime track_data_PTS;
  static GstClockTime last_marker_epoch = GST_CLOCK_TIME_NONE;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  track_data = (void *) map.data;
  track_data_PTS = buffer->pts;
  GST_DEBUG_OBJECT (elem, "PTS Track data %" GST_TIME_FORMAT, GST_TIME_ARGS (track_data_PTS));
  fprintf(tool_fps_6d, "%" GST_TIME_FORMAT ",", GST_TIME_ARGS (track_data_PTS));
  fprintf(track_fps_3d, "%" GST_TIME_FORMAT ",", GST_TIME_ARGS (track_data_PTS));
  fprintf(track_fps_2d, "%" GST_TIME_FORMAT ",", GST_TIME_ARGS (track_data_PTS));

  size_t num_tools = GetToolPosFromBuffer (track_data, tool_pos_buffer, tool_quat_buffer, MAX_NUM_MARKERS, &buffer_too_small);
  GST_DEBUG_OBJECT (elem, "num tools = %" G_GSIZE_FORMAT ", buffer_too_small = %d", num_tools, buffer_too_small);
  for (size_t i = 0; i < num_tools; i++)
  {
    GST_DEBUG_OBJECT (elem, "tool pos: %f %f %f", tool_pos_buffer[i].tx, tool_pos_buffer[i].ty, tool_pos_buffer[i].tz);
  }

  num_markers = GetMarkerDataFromBuffer (track_data, marker_pos_buffer, frame_numbers, time_specs, MAX_NUM_MARKERS, &buffer_too_small);
  GST_DEBUG_OBJECT (elem, "num markers = %" G_GSIZE_FORMAT ", buffer_too_small = %d", num_markers, buffer_too_small);

  for (size_t i = 0; i < num_markers; i++)
  {
    GstClockTime clock_time = GST_TIMESPEC_TO_TIME (time_specs[i]);
    gchar *track_epoch = get_epoch_timestamp_from_clocktime (clock_time);
    GST_DEBUG_OBJECT (elem, "marker: %f %f %f (before transform), frame number = %d, Unix Epoch %s", marker_pos_buffer[i].tx, marker_pos_buffer[i].ty, marker_pos_buffer[i].tz, frame_numbers[i], track_epoch);
        g_free (track_epoch);
    fprintf(track_fps_3d, "%f %f %f,", marker_pos_buffer[i].tx, marker_pos_buffer[i].ty, marker_pos_buffer[i].tz);
    if (i == num_markers - 1)
    {
      if (last_marker_epoch != GST_CLOCK_TIME_NONE)
      {
        GST_DEBUG_OBJECT (elem, "Track Data time difference last sample: %" G_GUINT64_FORMAT "ms", GST_TIME_AS_MSECONDS (clock_time) - GST_TIME_AS_MSECONDS (last_marker_epoch));
      }
      last_marker_epoch = clock_time;
    }
  }
  fprintf(track_fps_3d, "\n");

  if (num_tools > 0 || num_markers > 0)
  {
    elem->do_draw_markers = default_draw_marker;
    elem->marker_count = num_markers;
    elem->track_data_PTS = track_data_PTS;

    if (elem->gst_lens_param == NULL)
    {
      GST_WARNING_OBJECT (elem, "Lens Parameters required to transform markers to the screen have not been set");
    }
    else
    {
      for(size_t i = 0; i < num_tools; i++){
        Quat q1 = tool_quat_buffer[i];
        Quat q2;
        q2.q0 = q1.q0;
        q2.qx = -q1.qx;
        q2.qy = -q1.qy;
        q2.qz = -q1.qz;
        Quat result1 = hamilton_product(q1, offset[i]);
        Quat result2 = hamilton_product(result1, q2);
        tool_pos_buffer[i].tx += result2.qx;
        tool_pos_buffer[i].ty += result2.qy;
        tool_pos_buffer[i].tz += result2.qz;
        fprintf(tool_fps_6d, ",%f %f %f %f %f %f %f", tool_pos_buffer[i].tx, tool_pos_buffer[i].ty, tool_pos_buffer[i].tz
          , tool_quat_buffer[i].q0, tool_quat_buffer[i].qx, tool_quat_buffer[i].qy, tool_quat_buffer[i].qz
        );
      }
      fprintf(tool_fps_6d, "\n");
      
      TransformToolAndMarkers (tool_pos_buffer, (int) num_tools, marker_pos_buffer, (int) num_markers, &elem->project_points);

      for (size_t i = 0; i < num_markers; i++)
      {
        elem->markers[i].tx = marker_pos_buffer[i].tx;
        elem->markers[i].ty = marker_pos_buffer[i].ty;
        GST_DEBUG_OBJECT (elem, "Screen marker pos: %f %f", elem->markers[i].tx, elem->markers[i].ty);
        fprintf(track_fps_2d, ",%f %f", elem->markers[i].tx, elem->markers[i].ty);
      }

      for (size_t i = 0; i < num_tools; i++)
      {
        elem->toolpos[i].tx = tool_pos_buffer[i].tx;
        elem->toolpos[i].ty = tool_pos_buffer[i].ty;
        GST_DEBUG_OBJECT (elem, "Screen tool pos: %f %f", elem->toolpos[i].tx, elem->toolpos[i].ty);
        fprintf(track_fps_2d, ",%f %f", elem->toolpos[i].tx, elem->toolpos[i].ty);
        elem->markers[elem->marker_count].tx = tool_pos_buffer[i].tx;
        elem->markers[elem->marker_count].ty = tool_pos_buffer[i].ty;
        elem->marker_count++;
      }
      fprintf(track_fps_2d, "\n");
    }
  }

  gst_buffer_unmap (buffer, &map);
}

void register_local_plugin_track_overlay ()
{
  /* local plugins */
  gst_plugin_register_static (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    "trackoverlay", "NDI Track Overlay",
    gst_track_overlay_plugin_init, VERSION, "LGPL", PACKAGE, PACKAGE_NAME, PACKAGE_ORIGIN);
}