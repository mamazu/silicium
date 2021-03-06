DELEGATOR_TEMPLATE(class Element, class Error)
DELEGATOR_NAME(Sink)
DELEGATOR_TYPEDEF(typedef Element element_type;)
DELEGATOR_TYPEDEF(typedef Error error_type;)
DELEGATOR_BASIC_METHOD(append, BOOST_PP_EMPTY(), error_type,
                       Si::array_view<element_type>)
DELEGATOR_BASIC_METHOD(append_n_times, BOOST_PP_EMPTY(), error_type,
                       Si::function<element_type()> const &, std::size_t)
DELEGATOR_BASIC_METHOD(max_size, const, Si::optional<boost::uint64_t>, Si::unit)
