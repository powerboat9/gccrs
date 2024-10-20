// { dg-options "-frust-name-resolution-2.0" }

pub mod foo {
    pub mod bar {
        fn f() {
            super::super::super::foo!(); // { dg-error "too many leading .super. keywords" }

            super::crate::foo!(); // { dg-error "failed to resolve: .crate. in paths can only be used" }

            crate::foo::bar::super::foo!(); // { dg-error "failed to resolve: .super. in paths can only be used" }
        }
    }
}
