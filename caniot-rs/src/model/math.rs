pub fn avg_of_slice_opt<T>(slice: &[Option<T>]) -> Option<f32>
where
    T: Clone + Into<f32>,
{
    let (sum, count) = slice
        .iter()
        .flatten()
        .fold((0.0, 0), |(acc, n), x| (acc + x.clone().into(), n + 1));

    if count == 0 {
        None
    } else {
        Some(sum / count as f32)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_avg_of_slice_opt() {
        let slice = vec![Some(1.0), Some(2.0), Some(3.0)];
        assert_eq!(avg_of_slice_opt(&slice), Some(2.0));

        let slice = vec![Some(1.0), None, Some(3.0)];
        assert_eq!(avg_of_slice_opt(&slice), Some(2.0));

        let slice: Vec<Option<f32>> = vec![None, None, None];
        assert_eq!(avg_of_slice_opt(&slice), None);
    }
}
