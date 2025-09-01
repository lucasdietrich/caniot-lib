use crate::controller::implementation::ControllerApi;

pub(crate) struct ControllerApiWrapper {
    data: Box<Box<dyn ControllerApi>>, // Box<dyn ..> is a fat pointer, so wrap it again in a Box
}

impl ControllerApiWrapper {
    pub fn new<A: ControllerApi + 'static>(api: A) -> ControllerApiWrapper {
        ControllerApiWrapper {
            data: Box::new(Box::new(api)),
        }
    }

    pub fn get_data(&mut self) -> *mut ::core::ffi::c_void {
        self.data.as_mut() as *mut Box<dyn ControllerApi> as *mut ::core::ffi::c_void
    }
}
