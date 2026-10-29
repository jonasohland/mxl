// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

// a pipeline clock that for some (possibly contrived) reason reads ahead of CLOCK_TAI makes `D` negative.
// `clock_offset` saturates `D` to 0, so the sink writes grains ahead of MXL now by that difference.

use std::time::Duration;

use gst::glib;
use gst::prelude::*;
use gstreamer as gst;
use mxl::Rational;

fn main() {
    gst::init().unwrap();
    gst::Element::register(
        None,
        "mxlsink",
        gst::Rank::NONE,
        gstmxl::mxlsink::MxlSink::static_type(),
    )
    .unwrap();
    let domain = format!("/dev/shm/mxl_repro_{}", uuid::Uuid::new_v4());
    std::fs::create_dir_all(&domain).unwrap();
    let api = mxl::load_api(mxl::config::get_mxl_so_path()).unwrap();
    let instance = mxl::MxlInstance::new(api, &domain, "").unwrap();
    let flow_id = uuid::Uuid::new_v4().to_string();

    let clock: gst::Clock = glib::Object::builder::<gst::SystemClock>()
        .property("clock-type", gst::ClockType::Tai)
        .build()
        .upcast();
    let internal = clock.internal_time();
    clock.set_calibration(internal, internal + gst::ClockTime::from_seconds(2), 1, 1);

    let pipeline = gst::parse::launch(&format!(
        "videotestsrc is-live=true ! video/x-raw,format=v210,width=2,height=2,framerate=25/1 \
         ! mxlsink flow-id={flow_id} domain={domain}"
    ))
    .unwrap()
    .downcast::<gst::Pipeline>()
    .unwrap();
    pipeline.use_clock(Some(&clock));
    pipeline.set_state(gst::State::Playing).unwrap();
    std::thread::sleep(Duration::from_secs(1));

    let reader = instance
        .create_flow_reader(&flow_id)
        .unwrap()
        .to_grain_reader()
        .unwrap();
    let head = reader.get_runtime_info().unwrap().headIndex as i128;
    let now = instance.get_current_index(&Rational {
        numerator: 25,
        denominator: 1,
    }) as i128;
    println!(
        "head - now = {:.2} s (expected about 0)",
        (head - now) as f64 / 25.0
    );

    drop(reader);
    pipeline.set_state(gst::State::Null).unwrap();
    std::fs::remove_dir_all(&domain).unwrap();
}
