// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

//! an element keeps its offset cell after it is stopped. when moved into
//! another pipeline, it still shares `D` with its old pipeline, although the two
//! pipelines run on different clocks.

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

    let flows = [
        uuid::Uuid::new_v4().to_string(),
        uuid::Uuid::new_v4().to_string(),
    ];
    let branches: Vec<gst::Bin> = flows
        .iter()
        .map(|flow_id| {
            gst::parse::bin_from_description(
                &format!(
                    "videotestsrc is-live=true ! video/x-raw,format=v210,width=2,height=2,framerate=25/1 \
                     ! mxlsink flow-id={flow_id} domain={domain}"
                ),
                false,
            )
            .unwrap()
        })
        .collect();

    let pipeline_b = gst::Pipeline::new();
    let realtime: gst::Clock = glib::Object::builder::<gst::SystemClock>()
        .property("clock-type", gst::ClockType::Realtime)
        .build()
        .upcast();
    pipeline_b.use_clock(Some(&realtime));
    pipeline_b.add_many(&branches).unwrap();
    pipeline_b.set_state(gst::State::Playing).unwrap();
    std::thread::sleep(Duration::from_secs(1));

    branches[1].set_state(gst::State::Null).unwrap();
    pipeline_b.remove(&branches[1]).unwrap();
    let pipeline_a = gst::Pipeline::new();
    pipeline_a.add(&branches[1]).unwrap();
    pipeline_a.set_state(gst::State::Playing).unwrap();
    std::thread::sleep(Duration::from_secs(1));

    let rate = Rational {
        numerator: 25,
        denominator: 1,
    };
    for (name, flow_id) in ["B", "A"].iter().zip(&flows) {
        let reader = instance
            .create_flow_reader(flow_id)
            .unwrap()
            .to_grain_reader()
            .unwrap();
        let head = reader.get_runtime_info().unwrap().headIndex as i128;
        let now = instance.get_current_index(&rate) as i128;
        println!(
            "flow in pipeline {name}: head - now = {:.2} s",
            (head - now) as f64 / 25.0
        );
    }

    pipeline_a.set_state(gst::State::Null).unwrap();
    pipeline_b.set_state(gst::State::Null).unwrap();
    std::fs::remove_dir_all(&domain).unwrap();
}
