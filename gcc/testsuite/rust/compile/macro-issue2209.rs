#[macro_use]
mod mod1 {
    #[macro_use]
    mod mod2 {
        macro_rules! a {
            () => ()
        }

    }

    a!();
}
